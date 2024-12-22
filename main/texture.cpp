#include "texture.hpp"

#include <cassert>

#include <stb_image.h>

#include "../support/error.hpp"

GLuint load_texture_2d(const char* aPath)
{
    assert(aPath);

    auto configureTextureParameters = [](GLuint target, GLenum pname, GLint param) {
        glTexParameteri(target, pname, param);
        };

    auto loadImageData = [&](const char* path, int& width, int& height, int& channels) {
        stbi_set_flip_vertically_on_load(true);
        stbi_uc* data = stbi_load(path, &width, &height, &channels, 4);
        if (!data) {
            throw Error("Unable to load image \"%s\"", path);
        }
        return data;
        };

    int imgWidth, imgHeight, imgChannels;
    stbi_uc* imgData = loadImageData(aPath, imgWidth, imgHeight, imgChannels);

    GLuint texHandle = 0;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
    stbi_image_free(imgData);

    auto applyMipmaps = [](GLuint target) {
        glGenerateMipmap(target);
        };
    applyMipmaps(GL_TEXTURE_2D);

    configureTextureParameters(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    configureTextureParameters(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    configureTextureParameters(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    configureTextureParameters(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    auto applyAnisotropy = [](GLuint target, float level) {
        glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY, level);
        };
    applyAnisotropy(GL_TEXTURE_2D, 6.f);

    return texHandle;
}