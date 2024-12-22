#include "cylinder.hpp"

SimpleMeshData make_cylinder(bool aCapped, std::size_t aSubdivs, Vec3f aColor, Mat44f aPreTransform)
{
    std::vector<Vec3f> positions, normalsList;

    auto computeAngle = [&](std::size_t i) {
        return (i + 1) / static_cast<float>(aSubdivs) * 6.283185307179586f; // 2 * PI
        };

    float lastY = std::cos(0.f), lastZ = std::sin(0.f);

    auto processCap = [&](bool front, float y, float z, float prevY, float prevZ, const Vec3f& normal) {
        auto base = front ? 0.f : 1.f;
        positions.push_back(Vec3f{ base, 0.f, 0.f });
        normalsList.push_back(normal);
        positions.push_back(Vec3f{ base, y, z });
        normalsList.push_back(normal);
        positions.push_back(Vec3f{ base, prevY, prevZ });
        normalsList.push_back(normal);
        };

    if (aCapped) {
        for (std::size_t i = 0; i < aSubdivs; ++i) {
            float angle = computeAngle(i);
            float currY = std::cos(angle), currZ = std::sin(angle);
            Vec3f sideNormal = normalize(Vec3f{ 1.f, currY, currZ });

            auto addRect = [&](float py, float pz, float cy, float cz) {
                positions.insert(positions.end(), {
                    Vec3f{ 0.f, py, pz }, Vec3f{ 0.f, cy, cz }, Vec3f{ 1.f, py, pz },
                    Vec3f{ 0.f, cy, cz }, Vec3f{ 1.f, cy, cz }, Vec3f{ 1.f, py, pz } });
                normalsList.insert(normalsList.end(), 6, sideNormal);
                };

            addRect(lastY, lastZ, currY, currZ);
            processCap(true, currY, currZ, lastY, lastZ, sideNormal);
            processCap(false, currY, currZ, lastY, lastZ, sideNormal);

            lastY = currY;
            lastZ = currZ;
        }
    }
    else {
        for (std::size_t i = 0; i < aSubdivs; ++i) {
            float angle = computeAngle(i);
            float currY = std::cos(angle), currZ = std::sin(angle);
            Vec3f sideNormal = normalize(Vec3f{ 1.f, currY, currZ });

            positions.insert(positions.end(), {
                Vec3f{ 0.f, lastY, lastZ }, Vec3f{ 0.f, currY, currZ }, Vec3f{ 1.f, lastY, lastZ },
                Vec3f{ 0.f, currY, currZ }, Vec3f{ 1.f, currY, currZ }, Vec3f{ 1.f, lastY, lastZ } });
            normalsList.insert(normalsList.end(), 6, sideNormal);

            lastY = currY;
            lastZ = currZ;
        }
    }

    auto transformVectors = [&](std::vector<Vec3f>& vecList, bool normalizeResult) {
        for (auto& vec : vecList) {
            Vec4f temp{ vec.x, vec.y, vec.z, 1.f };
            Vec4f result = aPreTransform * temp;
            if (normalizeResult) {
                vec = normalize(Vec3f{ result.x, result.y, result.z });
            }
            else {
                vec = Vec3f{ result.x / result.w, result.y / result.w, result.z / result.w };
            }
        }
        };

    transformVectors(positions, false);
    transformVectors(normalsList, true);

    SimpleMeshData cylinder;
    cylinder.positions = std::move(positions);
    cylinder.colors = decltype(cylinder.colors)(cylinder.positions.size(), aColor);
    cylinder.normals = std::move(normalsList);
    return cylinder;
}