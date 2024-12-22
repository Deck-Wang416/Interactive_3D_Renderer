#include "main.hpp"

//// Mesh rendering
//inline void mesh_renderer( GLuint vao, size_t vertexCount, 
//							GLuint textureObjectId,  
//							GLuint programID,  
//							Mat44f projCameraWorld, 
//							Mat33f normalMatrix 
//) {     
//	glUseProgram(programID);
//
//	// for camera
//	glUniformMatrix4fv(
//		0,
//		1, GL_TRUE,
//		projCameraWorld.v);
//
//	//for normals
//	glUniformMatrix3fv(
//		1,
//		1, GL_TRUE,
//		normalMatrix.v);
//
//	Vec3f lightDir = normalize(Vec3f{ 0.f, 1.f, -1.f });
//
//	glUniform3fv(2, 1, &lightDir.x);      // Ambient 
//	glUniform3f(3, 0.9f, 0.9f, 0.9f);	  // Diffusion
//	glUniform3f(4, 0.05f, 0.05f, 0.05f);  // Spectral
//
//	glBindVertexArray(vao);
//	if (textureObjectId != 0)
//	{
//		glActiveTexture(GL_TEXTURE0);
//		glBindTexture(GL_TEXTURE_2D, textureObjectId);
//	}
//	else
//	{
//		glBindTexture(GL_TEXTURE_2D, 0);
//	}
//
//	glDrawArrays(GL_TRIANGLES, 0, vertexCount);
//};;
//
