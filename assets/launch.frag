#version 430

in vec3 fragColor;
in vec3 fragNormal;

layout (location = 0) out vec3 fragOutput;

layout (location = 2) uniform vec3 lightDirection;
layout (location = 3) uniform vec3 diffuseLight;
layout (location = 4) uniform vec3 ambientLight;

void main()
{
    vec3 normalizedNormal = normalize(fragNormal);
    vec3 lightDirNorm = normalize(lightDirection);

    float lightIntensity = max(0.0, dot(normalizedNormal, lightDirNorm));
    fragOutput = (ambientLight + lightIntensity * diffuseLight) * fragColor;
}