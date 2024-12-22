#version 430

in vec3 fragColor;
in vec3 fragNormal;
in vec2 fragTexCoords;

layout (location=0) out vec3 fragOutput;

layout (location=2) uniform vec3 lightDirection;
layout (location=3) uniform vec3 lightDiffuseIntensity;
layout (location=4) uniform vec3 ambientLight;
layout (binding=0) uniform sampler2D textureSampler;

void main()
{
    // Normalize the normal vector
    vec3 normalizedNormal = normalize(fragNormal);

    // Calculate dot product for lighting
    float lightFactor = max(0.0, dot(normalizedNormal, lightDirection));

    // Compute final color
    vec3 texColor = texture(textureSampler, fragTexCoords).rgb;
    fragOutput = texColor * (ambientLight + lightFactor * lightDiffuseIntensity);
}