#version 430

layout (binding=1) uniform sampler2D uTexture;
//layout (location=0) out vec3 oColor;

out vec4 oColor;

void main()
{
	oColor = texture(uTexture, gl_PointCoord);
	//oColor = vec4(1.0, 0.0, 0.0, 1.0);
}