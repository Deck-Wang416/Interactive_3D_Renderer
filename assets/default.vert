#version 430

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexColor;
layout (location = 2) in vec3 vertexNormal;
layout (location = 3) in vec2 vertexTexCoords;

layout (location = 0) uniform mat4 projectionMatrix;
layout (location = 1) uniform mat3 normalTransform;

out vec3 fragColor;
out vec3 fragNormal;
out vec2 fragTexCoords;

void main()
{
    // Pass color directly to fragment shader
    fragColor = vertexColor;

    // Calculate vertex position in clip space
    gl_Position = projectionMatrix * vec4(vertexPosition, 1.0);

    // Transform normal vector and normalize it
    fragNormal = normalize(normalTransform * vertexNormal);

    // Pass texture coordinates to fragment shader
    fragTexCoords = vertexTexCoords;
}
