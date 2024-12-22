#include "simple_mesh.hpp"

SimpleMeshData concatenate( SimpleMeshData aM, SimpleMeshData const& aN )
{
	aM.positions.insert( aM.positions.end(), aN.positions.begin(), aN.positions.end() );
	aM.colors.insert( aM.colors.end(), aN.colors.begin(), aN.colors.end() );
	aM.normals.insert(aM.normals.end(), aN.normals.begin(), aN.normals.end());
	aM.textureCoords.insert(aM.textureCoords.end(), aN.textureCoords.begin(), aN.textureCoords.end());
	return aM;
}


GLuint create_vao(const SimpleMeshData& aMeshData)
{
    GLuint buffers[4] = { 0, 0, 0, 0 };
    GLuint vaoHandle = 0;

    auto generateAndBindBuffer = [](GLuint& buffer, GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
        glGenBuffers(1, &buffer);
        glBindBuffer(target, buffer);
        glBufferData(target, size, data, usage);
        };

    auto configureVertexAttrib = [](GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
        glVertexAttribPointer(index, size, type, normalized, stride, pointer);
        glEnableVertexAttribArray(index);
        };

    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    generateAndBindBuffer(buffers[0], GL_ARRAY_BUFFER, aMeshData.positions.size() * sizeof(Vec3f), aMeshData.positions.data(), GL_STATIC_DRAW);
    configureVertexAttrib(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    generateAndBindBuffer(buffers[1], GL_ARRAY_BUFFER, aMeshData.colors.size() * sizeof(Vec3f), aMeshData.colors.data(), GL_STATIC_DRAW);
    configureVertexAttrib(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    generateAndBindBuffer(buffers[2], GL_ARRAY_BUFFER, aMeshData.normals.size() * sizeof(Vec3f), aMeshData.normals.data(), GL_STATIC_DRAW);
    configureVertexAttrib(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    generateAndBindBuffer(buffers[3], GL_ARRAY_BUFFER, aMeshData.textureCoords.size() * sizeof(Vec2f), aMeshData.textureCoords.data(), GL_STATIC_DRAW);
    configureVertexAttrib(3, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    auto resetBindings = []() {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        };
    resetBindings();

    [[maybe_unused]] auto checkError = []() {
        glGetError(); // Intentional call for debugging (ignored)
        };
    checkError();

    return vaoHandle;
}