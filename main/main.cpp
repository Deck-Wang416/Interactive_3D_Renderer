#include <iostream>
#include <typeinfo>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>

#include <glad.h>
#include <GLFW/glfw3.h>

#include "../third_party/rapidobj/include/rapidobj/rapidobj.hpp"
#define STB_IMAGE_IMPLEMENTATION 
#include "../third_party/stb/include/stb_image.h"

#include "../support/error.hpp"
#include "../support/program.hpp"
#include "../support/checkpoint.hpp"
#include "../support/debug_output.hpp"
#include "../vmlib/vec4.hpp"
#include "../vmlib/mat44.hpp"
#include "defaults.hpp"

namespace
{
	constexpr char const* kWindowTitle = "COMP3811 - CW2";

	float camera_position[] = { 0.0f, 0.0f, 3.0f };
	float camera_yaw = 0.0f;
	float camera_pitch = 0.0f;

	bool mouse_look_enabled = false;
	double last_mouse_x = 0.0, last_mouse_y = 0.0;

	GLuint vao, vbo;
	GLuint shaderProgram;
	GLuint textureID;

	GLuint launchpad_vao, launchpad_vbo;
	GLuint launchpadShaderProgram;

	std::vector<float> positions;
	std::vector<float> normals;
	std::vector<float> texCoords;

	std::vector<float> launchpad_positions;
	std::vector<float> launchpad_normals;

	void load_texture(const std::string& path) {
		int width, height, nrChannels;
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
		if (!data) {
			throw std::runtime_error("Failed to load texture: " + path);
		}

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (nrChannels == 3) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		} else if (nrChannels == 4) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		} else {
			throw std::runtime_error("Unsupported texture format.");
		}

		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(data);
	}

	void glfw_callback_mouse_button(GLFWwindow* window, int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
			mouse_look_enabled = !mouse_look_enabled;

			if (mouse_look_enabled) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			} else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}
	}

	void glfw_callback_cursor_position(GLFWwindow* window, double xpos, double ypos) {
		if (!mouse_look_enabled) return;

		float sensitivity = 0.1f;
		float dx = xpos - last_mouse_x;
		float dy = last_mouse_y - ypos;

		last_mouse_x = xpos;
		last_mouse_y = ypos;

		dx *= sensitivity;
		dy *= sensitivity;

		camera_yaw += dx;
		camera_pitch += dy;

		if (camera_pitch > 89.0f) camera_pitch = 89.0f;
		if (camera_pitch < -89.0f) camera_pitch = -89.0f;
	}
	
	void glfw_callback_error_( int, char const* );

	void glfw_callback_key_( GLFWwindow*, int, int, int, int );

	void load_launchpad_model(const std::string& path)
	{
		auto result = rapidobj::ParseFile(path);
		if (result.error) {
			throw std::runtime_error("Failed to load OBJ file: " + path);
		}

		launchpad_positions.clear();
		launchpad_normals.clear();

		for (const auto& shape : result.shapes) {
			for (const auto& index : shape.mesh.indices) {
				int vertex_index = index.position_index;
				launchpad_positions.push_back(result.attributes.positions[vertex_index * 3 + 0]);
				launchpad_positions.push_back(result.attributes.positions[vertex_index * 3 + 1]);
				launchpad_positions.push_back(result.attributes.positions[vertex_index * 3 + 2]);

				if (index.normal_index >= 0) {
					int normal_index = index.normal_index;
					launchpad_normals.push_back(result.attributes.normals[normal_index * 3 + 0]);
					launchpad_normals.push_back(result.attributes.normals[normal_index * 3 + 1]);
					launchpad_normals.push_back(result.attributes.normals[normal_index * 3 + 2]);
				}
			}
		}

		glGenVertexArrays(1, &launchpad_vao);
		glBindVertexArray(launchpad_vao);

		glGenBuffers(1, &launchpad_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, launchpad_vbo);

		std::vector<float> vertex_data;
		for (size_t i = 0; i < launchpad_positions.size() / 3; ++i) {
			vertex_data.push_back(launchpad_positions[i * 3 + 0]);
			vertex_data.push_back(launchpad_positions[i * 3 + 1]);
			vertex_data.push_back(launchpad_positions[i * 3 + 2]);

			vertex_data.push_back(launchpad_normals[i * 3 + 0]);
			vertex_data.push_back(launchpad_normals[i * 3 + 1]);
			vertex_data.push_back(launchpad_normals[i * 3 + 2]);
		}

		glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(float), vertex_data.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void load_obj_model(const std::string& path)
	{
		auto result = rapidobj::ParseFile(path);
		if (result.error) {
			throw std::runtime_error("Failed to load OBJ file: " + path);
		}

		positions.clear();
		normals.clear();
		texCoords.clear();

		for (const auto& shape : result.shapes) {
			for (const auto& index : shape.mesh.indices) {
				int vertex_index = index.position_index;
				positions.push_back(result.attributes.positions[vertex_index * 3 + 0]);
				positions.push_back(result.attributes.positions[vertex_index * 3 + 1]);
				positions.push_back(result.attributes.positions[vertex_index * 3 + 2]);

				if (index.normal_index >= 0) {
					int normal_index = index.normal_index;
					normals.push_back(result.attributes.normals[normal_index * 3 + 0]);
					normals.push_back(result.attributes.normals[normal_index * 3 + 1]);
					normals.push_back(result.attributes.normals[normal_index * 3 + 2]);
				}

				if (index.texcoord_index >= 0) {
					int texcoord_index = index.texcoord_index;
					texCoords.push_back(result.attributes.texcoords[texcoord_index * 2 + 0]);
					texCoords.push_back(result.attributes.texcoords[texcoord_index * 2 + 1]);
				} else {
					texCoords.push_back(0.0f);
					texCoords.push_back(0.0f);
				}
			}
		}

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		std::vector<float> vertex_data;
		for (size_t i = 0; i < positions.size() / 3; ++i) {
			vertex_data.push_back(positions[i * 3 + 0]);
			vertex_data.push_back(positions[i * 3 + 1]);
			vertex_data.push_back(positions[i * 3 + 2]);

			vertex_data.push_back(normals[i * 3 + 0]);
			vertex_data.push_back(normals[i * 3 + 1]);
			vertex_data.push_back(normals[i * 3 + 2]);

			vertex_data.push_back(texCoords[i * 2 + 0]);
			vertex_data.push_back(texCoords[i * 2 + 1]);
		}

		glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(float), vertex_data.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); 
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	void glfw_callback_error_(int, char const*);
	void glfw_callback_key_(GLFWwindow*, int, int, int, int);

	struct GLFWCleanupHelper
	{
		~GLFWCleanupHelper();
	};
	struct GLFWWindowDeleter
	{
		~GLFWWindowDeleter();
		GLFWwindow* window;
	};
}

void render_scene()
{
    glUseProgram(shaderProgram); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mat44f view_matrix = make_translation({ -camera_position[0], -camera_position[1], -camera_position[2] });
    Mat44f rotation_x = make_rotation_x(camera_pitch * M_PI / 180.0f);
    Mat44f rotation_y = make_rotation_y(camera_yaw * M_PI / 180.0f);
    Mat44f view = rotation_x * rotation_y * view_matrix;

    GLint view_loc = glGetUniformLocation(shaderProgram, "view");
    if (view_loc != -1) {
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.v);
    }

    Vec3f lightDirection = { 0.0f, 1.0f, -1.0f };
    float length = std::sqrt(lightDirection[0] * lightDirection[0] + 
                             lightDirection[1] * lightDirection[1] + 
                             lightDirection[2] * lightDirection[2]);
    lightDirection[0] /= length;
    lightDirection[1] /= length;
    lightDirection[2] /= length;

    GLint lightDirLoc = glGetUniformLocation(shaderProgram, "lightDir");
    if (lightDirLoc != -1) {
        glUniform3fv(lightDirLoc, 1, &lightDirection[0]);
    }

    Vec3f lightColor = { 1.0f, 1.0f, 1.0f };
    GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    if (lightColorLoc != -1) {
        glUniform3fv(lightColorLoc, 1, &lightColor[0]);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    glBindVertexArray(vao);
    int vertex_count = positions.size() / 3;
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(launchpadShaderProgram); 

    GLint view_loc_launchpad = glGetUniformLocation(launchpadShaderProgram, "view");
    if (view_loc_launchpad != -1) {
        glUniformMatrix4fv(view_loc_launchpad, 1, GL_FALSE, view.v);
    }

    GLint lightDirLoc_launchpad = glGetUniformLocation(launchpadShaderProgram, "lightDir");
    if (lightDirLoc_launchpad != -1) {
        glUniform3fv(lightDirLoc_launchpad, 1, &lightDirection[0]);
    }

    GLint lightColorLoc_launchpad = glGetUniformLocation(launchpadShaderProgram, "lightColor");
    if (lightColorLoc_launchpad != -1) {
        glUniform3fv(lightColorLoc_launchpad, 1, &lightColor[0]);
    }

    glBindVertexArray(launchpad_vao);

	// first launchpad
    Mat44f model_1 = make_translation({ 10.0f, 0.05f, -15.0f });
    GLint model_loc = glGetUniformLocation(launchpadShaderProgram, "model");
    if (model_loc != -1) {
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, model_1.v);
    }
    int launchpad_vertex_count = launchpad_positions.size() / 3;
    glDrawArrays(GL_TRIANGLES, 0, launchpad_vertex_count);

    // second launchpad
    Mat44f model_2 = make_translation({ -20.0f, 0.05f, 25.0f });
    if (model_loc != -1) {
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, model_2.v);
    }
    glDrawArrays(GL_TRIANGLES, 0, launchpad_vertex_count);

    glBindVertexArray(0);
}

GLuint create_shader_program(const char* vertex_shader_source, const char* fragment_shader_source)
{
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertex_shader);

    GLint success;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertex_shader, 512, nullptr, infoLog);
        throw std::runtime_error("Vertex Shader Compilation Error:\n" + std::string(infoLog));
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragment_shader, 512, nullptr, infoLog);
        throw std::runtime_error("Fragment Shader Compilation Error:\n" + std::string(infoLog));
    }

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shader_program, 512, nullptr, infoLog);
        throw std::runtime_error("Shader Program Linking Error:\n" + std::string(infoLog));
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

int main() try
{
	// Initialize GLFW
	if( GLFW_TRUE != glfwInit() )
	{
		char const* msg = nullptr;
		int ecode = glfwGetError( &msg );
		throw Error( "glfwInit() failed with '%s' (%d)", msg, ecode );
	}

	// Ensure that we call glfwTerminate() at the end of the program.
	GLFWCleanupHelper cleanupHelper;

	// Configure GLFW and create window
	glfwSetErrorCallback( &glfw_callback_error_ );

	glfwWindowHint( GLFW_SRGB_CAPABLE, GLFW_TRUE );
	glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );
	//glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

	glfwWindowHint( GLFW_DEPTH_BITS, 24 );

#	if !defined(NDEBUG)
	// When building in debug mode, request an OpenGL debug context. This
	// enables additional debugging features. However, this can carry extra
	// overheads. We therefore do not do this for release builds.
	glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );
#	endif // ~ !NDEBUG

	GLFWwindow* window = glfwCreateWindow(
		1280,
		720,
		kWindowTitle,
		nullptr, nullptr
	);

	if( !window )
	{
		char const* msg = nullptr;
		int ecode = glfwGetError( &msg );
		throw Error( "glfwCreateWindow() failed with '%s' (%d)", msg, ecode );
	}

	GLFWWindowDeleter windowDeleter{ window };

	// Set up event handling
	glfwSetKeyCallback( window, &glfw_callback_key_ );
	glfwSetMouseButtonCallback(window, glfw_callback_mouse_button);
	glfwSetCursorPosCallback(window, glfw_callback_cursor_position);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
		if (!mouse_look_enabled) return;

		float sensitivity = 0.1f;
		float dx = xpos - last_mouse_x;
		float dy = last_mouse_y - ypos;
		last_mouse_x = xpos;
		last_mouse_y = ypos;

		dx *= sensitivity;
		dy *= sensitivity;

		camera_yaw += dx;
		camera_pitch += dy;

		if (camera_pitch > 89.0f) camera_pitch = 89.0f;
		if (camera_pitch < -89.0f) camera_pitch = -89.0f;
	});

	// Set up drawing stuff
	glfwMakeContextCurrent( window );
	glfwSwapInterval( 1 ); // V-Sync is on.

	// Initialize GLAD
	// This will load the OpenGL API. We mustn't make any OpenGL calls before this!
	if( !gladLoadGLLoader( (GLADloadproc)&glfwGetProcAddress ) )
		throw Error( "gladLoaDGLLoader() failed - cannot load GL API!" );

	std::printf( "RENDERER %s\n", glGetString( GL_RENDERER ) );
	std::printf( "VENDOR %s\n", glGetString( GL_VENDOR ) );
	std::printf( "VERSION %s\n", glGetString( GL_VERSION ) );
	std::printf( "SHADING_LANGUAGE_VERSION %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ) );

	// Debug output
#	if !defined(NDEBUG)
	setup_gl_debug_output();
#	endif // ~ !NDEBUG

	// Global GL state
	OGL_CHECKPOINT_ALWAYS();

	load_obj_model("../assets/parlahti.obj");
	load_texture("../assets/L4343A-4k.jpeg");
	load_launchpad_model("../assets/landingpad.obj"); 

	OGL_CHECKPOINT_ALWAYS();

	// Get actual framebuffer size.
	// This can be different from the window size, as standard window
	// decorations (title bar, borders, ...) may be included in the window size
	// but not be part of the drawable surface area.
	int iwidth, iheight;
	glfwGetFramebufferSize( window, &iwidth, &iheight );
	float aspect_ratio = float(iwidth) / float(iheight);
	
	const char* vertex_shader_launchpad_source = R"(
		#version 330 core

		layout(location = 0) in vec3 aPos;
		layout(location = 1) in vec3 aNormal;

		out vec3 FragPos; 
		out vec3 Normal;

		uniform mat4 view;

		void main()
		{
			gl_Position = view * vec4(aPos, 1.0);
			FragPos = aPos;
			Normal = aNormal;
		}
	)";

	const char* fragment_shader_launchpad_source = R"(
		#version 330 core

		in vec3 FragPos;
		in vec3 Normal;

		out vec4 FragColor;

		uniform vec3 lightDir = vec3(0.0, 1.0, -1.0);
		uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
		uniform vec3 materialColor; // 材质颜色

		void main()
		{
			vec3 norm = normalize(Normal); 
			vec3 lightDirNorm = normalize(lightDir);
			float diff = max(dot(norm, lightDirNorm), 0.0);
			vec3 result = diff * lightColor * materialColor;
			FragColor = vec4(result, 1.0);
		}
	)";

	const char* vertex_shader_source = R"(
		#version 330 core

		layout(location = 0) in vec3 aPos;
		layout(location = 1) in vec3 aNormal;
		layout(location = 2) in vec2 aTexCoords;

		out vec3 FragPos; 
		out vec3 Normal;
		out vec2 TexCoords;

		uniform mat4 view;

		void main()
		{
			gl_Position = view * vec4(aPos, 1.0);
			FragPos = aPos;
			Normal = aNormal;
			TexCoords = aTexCoords;
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330 core

		in vec3 Normal;
		in vec2 TexCoords;

		out vec4 FragColor;

		uniform vec3 lightDir = vec3(0.0, 1.0, -1.0);
		uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
		uniform sampler2D texture1;

		void main()
		{
			vec3 norm = normalize(Normal); 
			vec3 lightDirNorm = normalize(lightDir);
			float diff = max(dot(norm, lightDirNorm), 0.0);
			vec4 texColor = texture(texture1, TexCoords); 
			FragColor = texColor * vec4(vec3(diff), 1.0);
		}
	)";

	launchpadShaderProgram = create_shader_program(vertex_shader_launchpad_source, fragment_shader_launchpad_source);
	shaderProgram = create_shader_program(vertex_shader_source, fragment_shader_source);

	glViewport( 0, 0, iwidth, iheight );

	glClearColor(0.4f, 0.4f, 0.4f, 1.0f);

	glEnable(GL_DEPTH_TEST);

	// Other initialization & loading
	OGL_CHECKPOINT_ALWAYS();

	// Main loop
	while( !glfwWindowShouldClose( window ) )
	{
		// Let GLFW process events
		glfwPollEvents();
		
		// Check if window was resized.
		float fbwidth, fbheight;
		{
			int nwidth, nheight;
			glfwGetFramebufferSize( window, &nwidth, &nheight );

			fbwidth = float(nwidth);
			fbheight = float(nheight);

			if( 0 == nwidth || 0 == nheight )
			{
				// Window minimized? Pause until it is unminimized.
				// This is a bit of a hack.
				do
				{
					glfwWaitEvents();
					glfwGetFramebufferSize( window, &nwidth, &nheight );
				} while( 0 == nwidth || 0 == nheight );
			}

			glViewport( 0, 0, nwidth, nheight );
		}
		// Draw scene
		OGL_CHECKPOINT_DEBUG();

		render_scene();

		// Display results
		glfwSwapBuffers( window );
	}

	return 0;
}
catch( std::exception const& eErr )
{
	std::fprintf( stderr, "Top-level Exception (%s):\n", typeid(eErr).name() );
	std::fprintf( stderr, "%s\n", eErr.what() );
	std::fprintf( stderr, "Bye.\n" );
	return 1;
}

namespace
{
	void glfw_callback_error_( int aErrNum, char const* aErrDesc )
	{
		std::fprintf( stderr, "GLFW error: %s (%d)\n", aErrDesc, aErrNum );
	}

	void glfw_callback_key_( GLFWwindow* aWindow, int aKey, int, int aAction, int )
	{
		if( GLFW_KEY_ESCAPE == aKey && GLFW_PRESS == aAction )
		{
			glfwSetWindowShouldClose( aWindow, GLFW_TRUE );
			return;
		}

		if (aAction == GLFW_PRESS || aAction == GLFW_REPEAT) {
			float speed = 0.1f;
			if (glfwGetKey(aWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 2.0f;
			if (glfwGetKey(aWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 0.5f;

			if (aKey == GLFW_KEY_W) camera_position[2] -= speed;
			if (aKey == GLFW_KEY_S) camera_position[2] += speed;
			if (aKey == GLFW_KEY_A) camera_position[0] -= speed;
			if (aKey == GLFW_KEY_D) camera_position[0] += speed;
			if (aKey == GLFW_KEY_E) camera_position[1] += speed;
			if (aKey == GLFW_KEY_Q) camera_position[1] -= speed;
		}
	}

}

namespace
{
	GLFWCleanupHelper::~GLFWCleanupHelper()
	{
		glfwTerminate();
	}

	GLFWWindowDeleter::~GLFWWindowDeleter()
	{
		if( window )
			glfwDestroyWindow( window );
	}
}
