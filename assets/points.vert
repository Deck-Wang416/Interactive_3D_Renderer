#version 430

layout (location = 0) in vec3 vertexPosition;
layout (location = 0) uniform mat4 projectionMatrix;

void main()
{
    vec4 transformedPosition = vec4(vertexPosition, 1.0);
    gl_Position = projectionMatrix * transformedPosition;
    gl_PointSize = 10.0;
}