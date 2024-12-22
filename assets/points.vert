#version 430

layout (location = 0) in vec3 iPosition;
layout (location = 0) uniform mat4 uProjCameraWorld;

void main()
{
	gl_Position = uProjCameraWorld * vec4(iPosition, 1.0);
	//gl_Position = vec4(1.0, 1.0, 1.0, 1.0);
	//gl_Position = vec4(iPosition, 1.0);
	gl_PointSize = 10.0; 
}
