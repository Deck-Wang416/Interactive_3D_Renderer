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
class RenderMetricsTracker {
private:
    static const size_t QUERY_BUFFER_SIZE = 8;  // Total queries used for tracking
    static const size_t MAX_HISTORY_DEPTH = 5; // Maximum number of frames to track

    std::array<GLuint, QUERY_BUFFER_SIZE> gpuTimers;  // GPU timer queries
    std::deque<MetricsData> historyBuffer;           // History of recorded metrics
    bool featureEnabled;                             // Indicates support for GPU queries

    std::chrono::steady_clock::time_point frameStartTime;  // Frame start timestamp
    std::chrono::steady_clock::time_point commandStartTime; // Command submission start

    struct InternalStatus {
        bool initialized = false;
        bool errorOccurred = false;
    };

    InternalStatus statusFlags;

public:
    RenderMetricsTracker() : featureEnabled(false) {
        GLint timestampCapability = 0;
        glGetQueryiv(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &timestampCapability);

        if (timestampCapability == 0) {
            printf("[WARNING] GPU lacks support for GL_TIMESTAMP.\n");
            featureEnabled = false;
            return;
        }

        featureEnabled = true;

        glGenQueries(QUERY_BUFFER_SIZE, gpuTimers.data());
        if (glGetError() != GL_NO_ERROR) {
            printf("[ERROR] Failed to initialize GPU query objects.\n");
            featureEnabled = false;
            return;
        }

        for (size_t i = 0; i < MAX_HISTORY_DEPTH; ++i) {
            historyBuffer.emplace_back();
        }
        statusFlags.initialized = true;
    }

    ~RenderMetricsTracker() {
        if (featureEnabled) {
            glDeleteQueries(QUERY_BUFFER_SIZE, gpuTimers.data());
        }
    }

    void startFrame() {
        frameStartTime = std::chrono::steady_clock::now();
        if (featureEnabled) {
            glQueryCounter(gpuTimers[0], GL_TIMESTAMP);
        }
    }

    void beginTrackingEvent(GLuint eventIndex) {
        if (featureEnabled) {
            glQueryCounter(gpuTimers[eventIndex], GL_TIMESTAMP);
        }
    }

    void endTrackingEvent(GLuint eventIndex) {
        if (featureEnabled) {
            glQueryCounter(gpuTimers[eventIndex + 1], GL_TIMESTAMP);
        }
    }

    void recordCommandStart() {
        commandStartTime = std::chrono::steady_clock::now();
    }

    void completeFrame() {
        if (!featureEnabled) {
            return;
        }

        glQueryCounter(gpuTimers[1], GL_TIMESTAMP);
        auto frameEndTime = std::chrono::steady_clock::now();

        auto cpuFrameDuration = frameEndTime - frameStartTime;
        auto cpuCommandDuration = frameEndTime - commandStartTime;

        GLuint64 gpuStart = 0, gpuEnd = 0;
        GLuint64 gpuSectionStart1 = 0, gpuSectionEnd1 = 0;
        GLuint64 gpuSectionStart2 = 0, gpuSectionEnd2 = 0;
        GLuint64 gpuSectionStart3 = 0, gpuSectionEnd3 = 0;

        GLint resultAvailable = 0;
        int retryAttempts = 0;
        constexpr int MAX_RETRIES = 500;

        while (!resultAvailable && retryAttempts < MAX_RETRIES) {
            glGetQueryObjectiv(gpuTimers[1], GL_QUERY_RESULT_AVAILABLE, &resultAvailable);
            if (!resultAvailable) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                retryAttempts++;
            }
        }

        if (!resultAvailable) {
            printf("[ERROR] Timed out while waiting for query results.\n");
            return;
        }

        glGetQueryObjectui64v(gpuTimers[0], GL_QUERY_RESULT, &gpuStart);
        glGetQueryObjectui64v(gpuTimers[1], GL_QUERY_RESULT, &gpuEnd);
        glGetQueryObjectui64v(gpuTimers[2], GL_QUERY_RESULT, &gpuSectionStart1);
        glGetQueryObjectui64v(gpuTimers[3], GL_QUERY_RESULT, &gpuSectionEnd1);
        glGetQueryObjectui64v(gpuTimers[4], GL_QUERY_RESULT, &gpuSectionStart2);
        glGetQueryObjectui64v(gpuTimers[5], GL_QUERY_RESULT, &gpuSectionEnd2);
        glGetQueryObjectui64v(gpuTimers[6], GL_QUERY_RESULT, &gpuSectionStart3);
        glGetQueryObjectui64v(gpuTimers[7], GL_QUERY_RESULT, &gpuSectionEnd3);

        MetricsData frameMetrics{
            gpuEnd - gpuStart,
            gpuSectionEnd1 - gpuSectionStart1,
            gpuSectionEnd2 - gpuSectionStart2,
            gpuSectionEnd3 - gpuSectionStart3,
            std::chrono::duration_cast<std::chrono::nanoseconds>(cpuFrameDuration),
            std::chrono::duration_cast<std::chrono::nanoseconds>(cpuCommandDuration)
        };

        if (historyBuffer.size() >= MAX_HISTORY_DEPTH) {
            historyBuffer.pop_front();
        }
        historyBuffer.push_back(frameMetrics);
    }

    void displaySummary() const {
        if (!featureEnabled || historyBuffer.empty()) {
            return;
        }

        GLuint64 totalGpuTime = 0, time1 = 0, time2 = 0, time3 = 0;
        std::chrono::nanoseconds totalCpuFrameTime{0}, totalCpuCmdTime{0};

        for (const auto& metrics : historyBuffer) {
            totalGpuTime += metrics.gpuTotalTime;
            time1 += metrics.gpuTimeSectionA;
            time2 += metrics.gpuTimeSectionB;
            time3 += metrics.gpuTimeSectionC;
        }

        size_t trackedFrames = historyBuffer.size();

        printf("\n[METRICS REPORT]\n");
        printf("Frames: %zu\n", trackedFrames);
        printf("GPU: Total: %.2fms, A: %.2fms, B: %.2fms, C: %.2fms\n",
               (totalGpuTime / trackedFrames) / 1e6,
               (time1 / trackedFrames) / 1e6,
               (time2 / trackedFrames) / 1e6,
               (time3 / trackedFrames) / 1e6);
        printf("CPU: Frame: %.2fms, Commands: %.2fms\n",
               totalCpuFrameTime.count() / (trackedFrames * 1e6),
               totalCpuCmdTime.count() / (trackedFrames * 1e6));
    }
};

#endif
#endif