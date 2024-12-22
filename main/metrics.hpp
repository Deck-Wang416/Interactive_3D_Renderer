#ifndef SYSTEM_METRICS_ANALYZER_HPP
#define SYSTEM_METRICS_ANALYZER_HPP

#include <glad.h>
#include <array>
#include <deque>
#include <chrono>
#include <thread>
#include <cstdio>

#define METRICS_COLLECTION_ENABLED

#ifdef METRICS_COLLECTION_ENABLED
struct MetricsData {
    GLuint64 gpuTotalTime;
    GLuint64 gpuTimeSectionA;
    GLuint64 gpuTimeSectionB;
    GLuint64 gpuTimeSectionC;
    std::chrono::nanoseconds cpuTimeFrame;
    std::chrono::nanoseconds cpuTimeCommands;
};

// task1.12
class MetricsAnalyzer {
private:
    static constexpr size_t NUM_QUERIES = 8;
    static constexpr size_t METRIC_HISTORY_LIMIT = 5;

    std::array<GLuint, NUM_QUERIES> queryIDs;
    std::deque<MetricsData> metricRecords;
    bool isSupported;

    std::chrono::steady_clock::time_point timeFrameStart;
    std::chrono::steady_clock::time_point timeCommandStart;

public:
    MetricsAnalyzer() : isSupported(false) {
        GLint counterCapabilities = 0;
        glGetQueryiv(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &counterCapabilities);

        if (counterCapabilities == 0) {
            printf("\n[!] METRIC SYSTEM NOT AVAILABLE: GPU lacks GL_TIMESTAMP support.\n");
            return;
        }

        isSupported = true;
        glGenQueries(NUM_QUERIES, queryIDs.data());
        if (glGetError() != GL_NO_ERROR) {
            printf("\n[!] METRIC INIT FAILURE: Could not initialize query objects.\n");
            isSupported = false;
            return;
        }

        for (size_t i = 0; i < METRIC_HISTORY_LIMIT; ++i) {
            metricRecords.emplace_back();
        }
    }

    ~MetricsAnalyzer() {
        if (isSupported) {
            glDeleteQueries(NUM_QUERIES, queryIDs.data());
        }
    }

    void recordFrameStart() {
        timeFrameStart = std::chrono::steady_clock::now();
        if (isSupported) {
            glQueryCounter(queryIDs[0], GL_TIMESTAMP);
        }
    }

    void recordEventStart(GLuint queryIndex) {
        if (isSupported) {
            glQueryCounter(queryIDs[queryIndex], GL_TIMESTAMP);
        }
    }

    void recordEventEnd(GLuint queryIndex) {
        if (isSupported) {
            glQueryCounter(queryIDs[queryIndex + 1], GL_TIMESTAMP);
        }
    }

    void recordCommandSubmission() {
        timeCommandStart = std::chrono::steady_clock::now();
    }

    void recordFrameEnd() {
        if (!isSupported) {
            return;
        }

        glQueryCounter(queryIDs[1], GL_TIMESTAMP);
        auto timeFrameEnd = std::chrono::steady_clock::now();

        auto frameDurationCPU = timeFrameEnd - timeFrameStart;
        auto commandLatencyCPU = timeFrameEnd - timeCommandStart;

        GLuint64 startTotalGPU = 0, endTotalGPU = 0;
        GLuint64 sectionStartA = 0, sectionEndA = 0;
        GLuint64 sectionStartB = 0, sectionEndB = 0;
        GLuint64 sectionStartC = 0, sectionEndC = 0;

        GLint isAvailable = 0;
        int retryCount = 0;
        constexpr int MAX_RETRY_LIMIT = 500;

        while (!isAvailable && retryCount < MAX_RETRY_LIMIT) {
            glGetQueryObjectiv(queryIDs[1], GL_QUERY_RESULT_AVAILABLE, &isAvailable);
            if (!isAvailable) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                retryCount++;
            }
        }

        if (!isAvailable) {
            printf("\n[!] METRIC TIMEOUT: Failed to retrieve query results within retry limit.\n");
            return;
        }

        glGetQueryObjectui64v(queryIDs[0], GL_QUERY_RESULT, &startTotalGPU);
        glGetQueryObjectui64v(queryIDs[1], GL_QUERY_RESULT, &endTotalGPU);
        glGetQueryObjectui64v(queryIDs[2], GL_QUERY_RESULT, &sectionStartA);
        glGetQueryObjectui64v(queryIDs[3], GL_QUERY_RESULT, &sectionEndA);
        glGetQueryObjectui64v(queryIDs[4], GL_QUERY_RESULT, &sectionStartB);
        glGetQueryObjectui64v(queryIDs[5], GL_QUERY_RESULT, &sectionEndB);
        glGetQueryObjectui64v(queryIDs[6], GL_QUERY_RESULT, &sectionStartC);
        glGetQueryObjectui64v(queryIDs[7], GL_QUERY_RESULT, &sectionEndC);

        MetricsData metrics{
            endTotalGPU - startTotalGPU,
            sectionEndA - sectionStartA,
            sectionEndB - sectionStartB,
            sectionEndC - sectionStartC,
            std::chrono::duration_cast<std::chrono::nanoseconds>(frameDurationCPU),
            std::chrono::duration_cast<std::chrono::nanoseconds>(commandLatencyCPU)
        };

        if (metricRecords.size() >= METRIC_HISTORY_LIMIT) {
            metricRecords.pop_front();
        }
        metricRecords.push_back(metrics);
    }

    void printMetricsSummary() const {
        if (!isSupported || metricRecords.empty()) {
            return;
        }

        GLuint64 totalTimeGPU = 0, timeSectionA = 0, timeSectionB = 0, timeSectionC = 0;
        std::chrono::nanoseconds totalTimeFrameCPU{0}, totalTimeCmdCPU{0};

        for (const auto& record : metricRecords) {
            totalTimeGPU += record.gpuTotalTime;
            timeSectionA += record.gpuTimeSectionA;
            timeSectionB += record.gpuTimeSectionB;
            timeSectionC += record.gpuTimeSectionC;
            totalTimeFrameCPU += record.cpuTimeFrame;
            totalTimeCmdCPU += record.cpuTimeCommands;
        }

        size_t totalFrames = metricRecords.size();

        printf("\n===== PERFORMANCE METRICS REPORT =====\n");
        printf("> Tracked Frames: %zu\n", totalFrames);
        printf("> GPU Metrics:\n");
        printf("    - Overall Render Time: %.3f ms/frame\n", (totalTimeGPU / totalFrames) / 1000000.0);
        printf("    - Section A Time: %.3f ms/frame\n", (timeSectionA / totalFrames) / 1000000.0);
        printf("    - Section B Time: %.3f ms/frame\n", (timeSectionB / totalFrames) / 1000000.0);
        printf("    - Section C Time: %.3f ms/frame\n", (timeSectionC / totalFrames) / 1000000.0);
        printf("> CPU Metrics:\n");
        printf("    - Frame Processing Time: %.3f ms/frame\n", totalTimeFrameCPU.count() / (totalFrames * 1000000.0));
        printf("    - Command Submission Time: %.3f ms/frame\n", totalTimeCmdCPU.count() / (totalFrames * 1000000.0));
        printf("=======================================\n");
    }
};
#endif

#endif