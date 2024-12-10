#include <glad.h>
#include <GLFW/glfw3.h>

#include <typeinfo>
#include <stdexcept>

#include <cstdio>
#include <cstdlib>

#include "../support/error.hpp"
#include "../support/program.hpp"
#include "../support/checkpoint.hpp"
#include "../support/debug_output.hpp"

#include "../vmlib/vec4.hpp"
#include "../vmlib/mat44.hpp"
#include "../third_party/rapidobj/include/rapidobj/rapidobj.hpp"

#include "defaults.hpp"

#include "iostream"

namespace
{
	constexpr char const* kWindowTitle = "COMP3811 - CW2";

	float camera_position[] = { 0.0f, 0.0f, 3.0f };
	float camera_yaw = 0.0f;
	float camera_pitch = 0.0f;

	bool mouse_look_enabled = false;
	double last_mouse_x = 0.0, last_mouse_y = 0.0;

	GLuint vao, vbo;
	std::vector<float> positions;
	GLuint shaderProgram;
	
	void glfw_callback_error_( int, char const* );

	void glfw_callback_key_( GLFWwindow*, int, int, int, int );

	void load_obj_model(const std::string& path)
	{
		auto result = rapidobj::ParseFile(path);
		if (result.error) {
			throw std::runtime_error("Failed to load OBJ file: " + path);
		}else {
			std::cout << "OBJ file loaded successfully!" << std::endl;
		}

		positions.clear();
		positions.insert(positions.end(), result.attributes.positions.begin(), result.attributes.positions.end());

		std::cout << "positions.size() = " << positions.size() << std::endl;
		std::cout << "attributes.positions.size() = " << result.attributes.positions.size() << std::endl;
		std::cout << "Type of positions: " << typeid(positions).name() << std::endl;
		std::cout << "Type of result.attributes.positions: " << typeid(result.attributes.positions).name() << std::endl;

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

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

    glBindVertexArray(vao);
    int vertex_count = positions.size() / 3;
	glDrawArrays(GL_TRIANGLES, 0, vertex_count);
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
	// TODO: Additional event handling setup

	glfwSetKeyCallback( window, &glfw_callback_key_ );

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

	// Ddebug output
#	if !defined(NDEBUG)
	setup_gl_debug_output();
#	endif // ~ !NDEBUG

	// Global GL state
	OGL_CHECKPOINT_ALWAYS();

	load_obj_model("../assets/parlahti.obj");

	OGL_CHECKPOINT_ALWAYS();

	// Get actual framebuffer size.
	// This can be different from the window size, as standard window
	// decorations (title bar, borders, ...) may be included in the window size
	// but not be part of the drawable surface area.
	int iwidth, iheight;
	glfwGetFramebufferSize( window, &iwidth, &iheight );

	float aspect_ratio = float(iwidth) / float(iheight);
	Mat44f projection_matrix = make_perspective_projection(45.0f * M_PI / 180.0f, aspect_ratio, 0.1f, 100.0f);

	const char* vertex_shader_source = R"(
		#version 330 core
		layout(location = 0) in vec3 aPos;

		uniform mat4 view;

		void main()
		{
			gl_Position = view * vec4(aPos, 1.0);
		}
	)";

	const char* fragment_shader_source = R"(
		#version 330 core
		out vec4 FragColor;

		void main()
		{
			FragColor = vec4(0.8, 0.3, 0.02, 1.0); // 这里的颜色可自定义
		}
	)";

	shaderProgram = create_shader_program(vertex_shader_source, fragment_shader_source);
	std::cout << "shaderProgram = " << shaderProgram << std::endl;

	glViewport( 0, 0, iwidth, iheight );

	// Other initialization & loading
	OGL_CHECKPOINT_ALWAYS();
	
	// TODO: global GL setup goes here

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

		// Update state
		//TODO: update state

		// Draw scene
		OGL_CHECKPOINT_DEBUG();

		//TODO: draw frame

		OGL_CHECKPOINT_DEBUG();

		render_scene();

		// Display results
		glfwSwapBuffers( window );
	}

	// Cleanup.
	//TODO: additional cleanup
	
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
			float speed = 0.1f; // 默认速度
			if (glfwGetKey(aWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 2.0f; // 加速
			if (glfwGetKey(aWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 0.5f; // 减速

			if (aKey == GLFW_KEY_W) camera_position[2] -= speed; // 向前
			if (aKey == GLFW_KEY_S) camera_position[2] += speed; // 向后
			if (aKey == GLFW_KEY_A) camera_position[0] -= speed; // 左
			if (aKey == GLFW_KEY_D) camera_position[0] += speed; // 右
			if (aKey == GLFW_KEY_E) camera_position[1] += speed; // 上
			if (aKey == GLFW_KEY_Q) camera_position[1] -= speed; // 下
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
