#include "loadobj.hpp"

#include <rapidobj/rapidobj.hpp>

#include "../support/error.hpp"

SimpleMeshData load_wavefront_obj(char const* aPath)
{
    auto parseAndTriangulate = [&]() {
        auto objResult = rapidobj::ParseFile(aPath);
        if (objResult.error) {
            throw Error("Failed to load OBJ file: '%s'\nError: '%s'", aPath, objResult.error.code.message().c_str());
        }
        rapidobj::Triangulate(objResult);
        return objResult;
        };

    auto objData = parseAndTriangulate();

    SimpleMeshData meshData;
    auto& positions = meshData.positions;
    auto& normals = meshData.normals;
    auto& colors = meshData.colors;
    auto& texCoords = meshData.textureCoords;

    auto appendAttributes = [&](const rapidobj::Index& index, const rapidobj::Result& data, const rapidobj::Material& mat) {
        positions.emplace_back(Vec3f{
            data.attributes.positions[index.position_index * 3 + 0],
            data.attributes.positions[index.position_index * 3 + 1],
            data.attributes.positions[index.position_index * 3 + 2] });

        normals.emplace_back(Vec3f{
            data.attributes.normals[index.normal_index * 3 + 0],
            data.attributes.normals[index.normal_index * 3 + 1],
            data.attributes.normals[index.normal_index * 3 + 2] });

        colors.emplace_back(Vec3f{
            mat.ambient[0] + mat.diffuse[0],
            mat.ambient[1] + mat.diffuse[1],
            mat.ambient[2] + mat.diffuse[2] });

        texCoords.emplace_back(Vec2f{
            data.attributes.texcoords[index.texcoord_index * 2 + 0],
            data.attributes.texcoords[index.texcoord_index * 2 + 1] });
        };

    for (const auto& shape : objData.shapes) {
        for (std::size_t i = 0; i < shape.mesh.indices.size(); ++i) {
            auto& idx = shape.mesh.indices[i];
            const auto& material = objData.materials[shape.mesh.material_ids[i / 3]];
            appendAttributes(idx, objData, material);
        }
    }

    return meshData;
}

