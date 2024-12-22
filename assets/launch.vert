#version 430

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec3 vertexCol;
layout (location = 2) in vec3 vertexNorm;

layout (location = 0) uniform mat4 projMatrix;
layout (location = 1) uniform mat3 normTransform;

out vec3 fragColor;
out vec3 fragNormal;

void main()
{
    fragColor = vertexCol;
    vec4 transformedPos = vec4(vertexPos, 1.0);
    gl_Position = projMatrix * transformedPos;
    vec3 computedNorm = normTransform * vertexNorm;
    fragNormal = normalize(computedNorm);
}
