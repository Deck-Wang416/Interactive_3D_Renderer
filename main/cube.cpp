#include "cube.hpp"


SimpleMeshData make_cube(Vec3f aColor, Mat44f aPreTransform)
{
    std::vector<Vec3f> positions;
    std::vector<Vec3f> normalsList;

    // index
    auto v = [](float x, float y, float z) { return Vec3f{ x, y, z }; };
    std::vector<Vec3f> verts{
        v(0.5f, 0.5f, 0.5f), v(-0.5f, 0.5f, 0.5f), v(-0.5f, -0.5f, 0.5f), v(0.5f, -0.5f, 0.5f),
        v(0.5f, 0.5f, -0.5f), v(-0.5f, 0.5f, -0.5f), v(-0.5f, -0.5f, -0.5f), v(0.5f, -0.5f, -0.5f) };

    const std::vector<int> faceIndices[]{
        {0, 1, 2, 2, 3, 0}, {4, 0, 3, 3, 7, 4}, {5, 4, 7, 7, 6, 5},
        {1, 5, 6, 6, 2, 1}, {4, 5, 1, 1, 0, 4}, {3, 2, 6, 6, 7, 3} };

    auto fn = [](float x, float y, float z) { return Vec3f{ x, y, z }; };
    const Vec3f normals[]{
        fn(0.f, 0.f, 1.f), fn(1.f, 0.f, 0.f), fn(0.f, 0.f, -1.f),
        fn(-1.f, 0.f, 0.f), fn(0.f, 1.f, 0.f), fn(0.f, -1.f, 0.f) };

    for (size_t f = 0; f < sizeof(faceIndices) / sizeof(faceIndices[0]); ++f) {
        auto indices = faceIndices[f];
        for (auto idx : indices) {
            positions.emplace_back(verts[idx]);
            normalsList.emplace_back(normals[f]);
        }
    }

    // matrix transform
    auto transform = [&](Vec3f p) -> Vec3f {
        Vec4f tmp{ p.x, p.y, p.z, 1.f };
        Vec4f res = aPreTransform * tmp;
        return Vec3f{ res.x / res.w, res.y / res.w, res.z / res.w };
        };

    auto transformNormal = [&](Vec3f n) -> Vec3f {
        Vec4f tmp{ n.x, n.y, n.z, 1.f };
        Vec4f res = aPreTransform * tmp;
        return normalize(Vec3f{ res.x, res.y, res.z });
        };

    for (size_t i = 0; i < positions.size(); ++i) {
        positions[i] = transform(positions[i]);
        normalsList[i] = transformNormal(normalsList[i]);
    }

    SimpleMeshData result;
    result.positions = std::move(positions);
    result.colors = decltype(result.colors)(result.positions.size(), aColor);
    result.normals = std::move(normalsList);
    return result;
}
