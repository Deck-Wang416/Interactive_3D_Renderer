#version 430

layout (binding = 1) uniform sampler2D textureSampler;

out vec4 fragmentOutput;

void main()
{
    vec2 texCoords = gl_PointCoord;
    vec4 sampledColor = texture(textureSampler, texCoords);
    fragmentOutput = sampledColor;
}