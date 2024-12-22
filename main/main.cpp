#include <glad.h>
#include <GLFW/glfw3.h>

#include <typeinfo>
#include <stdexcept>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <random>

#include "../support/error.hpp"
#include "../support/program.hpp"
#include "../support/checkpoint.hpp"
#include "../support/debug_output.hpp"

#include "../vmlib/vec4.hpp"
#include "../vmlib/mat44.hpp"
#include "../vmlib/mat33.hpp"

#include "defaults.hpp"

#include "cylinder.hpp"
#include "cone.hpp"
#include "loadobj.hpp"
#include "simple_mesh.hpp"
#include "loadcustom.hpp"

#include "cube.hpp"
#include "texture.hpp"

#include "fontstash.h"

namespace
{
	constexpr char const* kWindowTitle = "COMP3811 - CW2";

	constexpr float kPi_ = 3.1415926f;

	float kMovementPerSecond_ = 5.f; // units per second
	float kMouseSensitivity_ = 0.01f; // radians per pixel
	struct State_ //struct for camera control
	{
		ShaderProgram* prog;

		struct CamCtrl_
		{
			bool cameraActive;
			bool actionZoomIn, actionZoomOut;
			bool actionZoomleft, actionZoomRight;
			bool actionMoveForward, actionMoveBackward;
			bool actionMoveLeft, actionMoveRight;
			bool actionMoveUp, actionMoveDown;

			float phi, theta;
			float radius;
			Vec3f movementVec;

			float lastX, lastY;
		} camControl;

		// spaceship
		bool moveUp = false;
		float spaceshipOrigin = 0.f;
		float spaceshipCurve = 0.f;
		float acceleration = 0.1f;
		float curve = 0.f;
	};

	void glfw_callback_error_(int, char const*);
	void glfw_callback_key_(GLFWwindow*, int, int, int, int);
	void glfw_callback_motion_(GLFWwindow*, double, double); //function for mouse motion

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


struct Sprite {
	Vec3f position;
	Vec3f velocity;
	float lifespan;
};

std::vector<Sprite> sprites;
GLuint texture, VBO, VAO;
int maxSprites = 6000;


void loadTexture() { 
	texture = load_texture_2d("assets/fire.jpg"); 
	glGetError();
}

void setupSpriteBuffers() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, maxSprites * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0); 
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

std::mt19937 createRandomEngine() {
	std::random_device rd;
	return std::mt19937(rd());
}

std::uniform_real_distribution<> createUniformDistribution(double min, double max) {
	return std::uniform_real_distribution<>(min, max);
}

float generateRandomValue(std::mt19937& gen, std::uniform_real_distribution<>& dis) {
	return dis(gen);
}

Vec3f computeDirection(float phi, float theta) {
	Vec3f randomDirection;
	randomDirection.x = sin(phi) * cos(theta);
	randomDirection.y = sin(phi) * sin(theta);
	randomDirection.z = cos(phi);
	return randomDirection;
}

Vec3f randomConicalDirection(Vec3f direction) {
	// Initialize random engine and distribution
	static std::mt19937 gen = createRandomEngine();
	static std::uniform_real_distribution<> dis(0, 1);

	// Generate random spherical coordinates
	float theta = 2 * kPi_ * generateRandomValue(gen, dis);
	float phi = acos(1 - generateRandomValue(gen, dis) * (1 - cos(45)));

	// Compute and return random direction
	return computeDirection(phi, theta);
}

void updateSpritePositions(const std::vector<Sprite>& sprites) {
	// Preallocate memory for positions
	std::vector<float> positions;
	positions.reserve(sprites.size() * 3);

	// Populate positions vector
	for (size_t i = 0; i < sprites.size(); ++i) {
		const Sprite& s = sprites[i];
		positions.emplace_back(s.position.x);
		positions.emplace_back(s.position.y);
		positions.emplace_back(s.position.z);
	}

	// Bind VBO and upload data
	GLsizeiptr dataSize = positions.size() * sizeof(float);
	const void* bufferData = positions.data();
	GLenum usage = GL_DYNAMIC_DRAW;

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, dataSize, bufferData, usage);

	// Unbind buffer
	GLenum target = GL_ARRAY_BUFFER;
	glBindBuffer(target, 0);
}

void generateSprites(Vec3f spaceshipPosition, int spriteAmount, Vec3f direction) {
	for (int i = 0; i < spriteAmount; i++)
	{
		Sprite sprite;
		sprite.position = spaceshipPosition;
		sprite.velocity = randomConicalDirection(direction);
		sprite.lifespan = 0.5f;
		sprites.emplace_back(sprite);
	}
}

void updateSprites(float dt) {
    for (size_t i = 0; i < sprites.size(); ++i) {
        sprites[i].position.x += sprites[i].velocity.x * dt;
        sprites[i].position.y += sprites[i].velocity.y * dt;
        sprites[i].position.z += sprites[i].velocity.z * dt;
        sprites[i].lifespan -= dt;
    }

    // Remove dead sprites (manual iteration)
    std::vector<Sprite> updatedSprites;
    for (size_t i = 0; i < sprites.size(); ++i) {
        if (sprites[i].lifespan > 0) {
            updatedSprites.push_back(sprites[i]);
        }
    }

    // Replace the original vector with the updated one
    sprites = updatedSprites;
}

void renderSprites(Mat44f project2World, GLuint shader) {
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
	glActiveTexture(GL_TEXTURE0); 
	glBindTexture(GL_TEXTURE_2D, texture);  
	glUseProgram(shader);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glUniformMatrix4fv(0,1, GL_TRUE,project2World.v);
	glUniform1i(1, 0); 
	glBindVertexArray(VAO);
	glDrawArrays(GL_POINTS, 0, sprites.size()); 
	glBindVertexArray(0); 
	glDisable(GL_BLEND);
}

inline SimpleMeshData spaceship() {

	// Color definitions with obfuscated variable names
	Vec3f legClr = { 1.f, 0.514f, 0.737f };
	Vec3f bodyClr = { 0.8f, 0.f, 0.404f };
	Vec3f baseClr = { 0.992f, 0.510f, 0.184f };
	Vec3f cubeClr = { 0.976f, 0.294f, 0.f };

	// Body creation
	SimpleMeshData bdyLeft = make_cone(false, size_t(64), bodyClr, make_translation({ 0.f, 2.5f, 0.f }));
	SimpleMeshData bdyRight = make_cone(false, size_t(64), bodyClr, make_translation({ 0.f, 2.5f, 0.f }) * make_rotation_z(angleToRadians(180)));
	SimpleMeshData mainBody = concatenate(bdyLeft, bdyRight);

	// Legs creation with intermediate results
	SimpleMeshData lOne = make_cylinder(true, size_t(64), legClr, make_translation({ 0.f, 2.5f, 0.f }) * make_rotation_z(angleToRadians(-45)) * make_scaling(2.f, 0.1f, 0.1f));
	SimpleMeshData tempOne = concatenate(mainBody, lOne);

	SimpleMeshData lTwo = make_cylinder(true, size_t(64), legClr, make_translation({ 0.f, 2.5f, 0.f }) * make_rotation_z(angleToRadians(-135)) * make_scaling(2.f, 0.1f, 0.1f));
	SimpleMeshData tempTwo = concatenate(tempOne, lTwo);

	SimpleMeshData lThree = make_cylinder(true, size_t(64), legClr, make_translation({ 0.f, 2.5f, 0.f }) * make_rotation_y(angleToRadians(90)) * make_rotation_z(angleToRadians(-45)) * make_scaling(2.f, 0.1f, 0.1f));
	SimpleMeshData tempThree = concatenate(tempTwo, lThree);

	SimpleMeshData lFour = make_cylinder(true, size_t(64), legClr, make_translation({ 0.f, 2.5f, 0.f }) * make_rotation_y(angleToRadians(-90)) * make_rotation_z(angleToRadians(-45)) * make_scaling(2.f, 0.1f, 0.1f));
	SimpleMeshData tempFour = concatenate(tempThree, lFour);

	// Feet creation
	SimpleMeshData fOne = make_cylinder(true, size_t(64), cubeClr, make_translation({ -1.3f, 1.f, 0.0f }) * make_scaling(0.4f, 0.4f, 0.4f));
	SimpleMeshData tempFive = concatenate(tempFour, fOne);

	SimpleMeshData fTwo = make_cylinder(true, size_t(64), cubeClr, make_translation({ 1.3f, 1.f, 0.0f }) * make_scaling(0.4f, 0.4f, 0.4f));
	SimpleMeshData tempSix = concatenate(tempFive, fTwo);

	SimpleMeshData fThree = make_cylinder(true, size_t(64), cubeClr, make_translation({ 0.f, 1.f, -1.3f }) * make_scaling(0.4f, 0.4f, 0.4f));
	SimpleMeshData tempSeven = concatenate(tempSix, fThree);

	SimpleMeshData fFour = make_cylinder(true, size_t(64), cubeClr, make_translation({ 0.0f, 1.f, 1.3f }) * make_scaling(0.4f, 0.4f, 0.4f));
	SimpleMeshData tempEight = concatenate(tempSeven, fFour);

	// Connector bars
	SimpleMeshData connOne = make_cube(baseClr, make_translation({ -0.f, 1.f, 1.2f }) * make_rotation_y(angleToRadians(90)) * make_scaling(2.4f, 0.1f, 0.1f));
	SimpleMeshData tempNine = concatenate(tempEight, connOne);

	SimpleMeshData connTwo = make_cube(baseClr, make_translation({ -1.2f, 1.f, 0.f }) * make_scaling(2.4f, 0.1f, 0.1f));
	SimpleMeshData finalSpaceship = concatenate(tempNine, connTwo);

	// Scale spaceship down
	for (int i = 0; (long unsigned int)i < finalSpaceship.positions.size(); ++i) {
		finalSpaceship.positions[i] *= 0.18;
	}

	return finalSpaceship;
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
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

	glfwWindowHint(GLFW_DEPTH_BITS, 24);

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
	State_ state{};

	//TODO: Additional event handling setup
	//setting up keyboard event handling
	glfwSetWindowUserPointer(window, &state);
	glfwSetKeyCallback(window, &glfw_callback_key_);
	glfwSetCursorPosCallback(window, &glfw_callback_motion_);
	//endofTODO

	glfwSetKeyCallback( window, &glfw_callback_key_ );

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

	// TODO: global GL setup goes here
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2f, 0.2f, 0.2f, 0.2f);
	//endofTODO

	OGL_CHECKPOINT_ALWAYS();

	// Get actual framebuffer size.
	// This can be different from the window size, as standard window
	// decorations (title bar, borders, ...) may be included in the window size
	// but not be part of the drawable surface area.
	int iwidth, iheight;
	glfwGetFramebufferSize( window, &iwidth, &iheight );

	glViewport( 0, 0, iwidth, iheight );

	//load shader program
	ShaderProgram prog({
		{ GL_VERTEX_SHADER, "assets/default.vert" },
		{ GL_FRAGMENT_SHADER, "assets/default.frag" }
		});

	state.prog = &prog;
	state.camControl.radius = 10.f;

	auto last = Clock::now();
	float angle = 0.f;

	auto parlahti = load_wavefront_obj("assets/parlahti.obj");
	
	GLuint vao = create_vao(parlahti);
	std::size_t vertexCount = parlahti.positions.size();


	GLuint textures = load_texture_2d("assets/L4343A-4k.jpeg");

	ShaderProgram prog2 = ShaderProgram({
		{GL_VERTEX_SHADER, "assets/launch.vert"},
		{GL_FRAGMENT_SHADER, "assets/launch.frag"}
		});

	SimpleMeshData launch = load_wavefront_obj("assets/landingpad.obj");
	std::size_t launchVertexCount = launch.positions.size();
	std::vector<Vec3f> originalPositions(launch.positions.begin(), launch.positions.end());

	auto adjustLaunchPositions = [&](SimpleMeshData& mesh, Vec3f offset) {
		for (Vec3f& position : mesh.positions) {
			position = position + offset;
		}
		};

	adjustLaunchPositions(launch, Vec3f{ 0.f, -0.975f, -60.f });

	GLuint launch_vao_1 = create_vao(launch);

	// Reset positions
	launch.positions.assign(originalPositions.begin(), originalPositions.end());

	// Move the second launch object
	adjustLaunchPositions(launch, Vec3f{ -20.f, -0.975f, -10.f });

	// Create VAO for the second launchpad
	GLuint launch_vao_2 = create_vao(launch);


	 auto ship = spaceship();
	 size_t shipVertexCount = ship.positions.size();


	 for (size_t i = 0; i < shipVertexCount; i++)
	 {
		 ship.positions[i] = ship.positions[i] + Vec3f{ -20.f, -1.125f, -10.f };
	 }

	 // Create VAO for the ship
	 GLuint ship_one_vao = create_vao(ship);
	 ShaderProgram prog3({
			 { GL_VERTEX_SHADER, "assets/points.vert" },
			 { GL_FRAGMENT_SHADER, "assets/points.frag" }
	 });
	 loadTexture();
	 setupSpriteBuffers();
	 


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

		//TODO: update state
		auto calculateDeltaTime = [&](Clock::time_point& lastTime) {
			auto now = Clock::now();
			float deltaTime = std::chrono::duration_cast<Secondsf>(now - lastTime).count();
			lastTime = now;
			return deltaTime;
			};
		float dt = calculateDeltaTime(last);

		angle += dt * kPi_ * 0.3f;
		if (angle >= 2.f * kPi_) {
			angle -= 2.f * kPi_;
		}

		Mat44f model2World = make_rotation_y(0);

		auto updateSpaceshipWorldMatrix = [&]() -> Mat44f {
			if (!state.moveUp) return model2World;

			state.spaceshipOrigin += state.acceleration * dt;
			state.acceleration *= 1.0025;

			if (state.spaceshipOrigin > 0.5f) {
				state.spaceshipCurve += (0.05f * dt);
				state.spaceshipCurve *= 1.005f;
			}

			// Calculate rotation angle for spaceship
			auto calculateRotationAngle = [](float curve, float origin) -> float {
				return std::atan2(curve, origin);
				};
			float angleX = calculateRotationAngle(state.spaceshipCurve, state.spaceshipOrigin);

			Mat44f translationToOrigin = make_translation(Vec3f{ 20.f, 1.125f, 15.f });
			Mat44f xRotationMatrix = make_rotation_x(angleX);
			Mat44f originToTranslation = make_translation(Vec3f{ -20.f, -1.125f, -15.f });
			Mat44f translationMatrix = make_translation(Vec3f{ 0.f, state.spaceshipOrigin, state.spaceshipCurve });

			auto generateMultipleSprites = [&](const Vec3f& basePos, float offsetX, float offsetY, float offsetZ, int count, const Vec3f& negOffset) {
				generateSprites(basePos + Vec3f{ offsetX, offsetY, offsetZ }, count, negOffset);
				};

			// Generate sprites with offsets
			Vec3f basePosition = Vec3f{ 0.f, state.spaceshipOrigin, state.spaceshipCurve };
			Vec3f negOffset = Vec3f{ 0.f, -state.spaceshipOrigin, -state.spaceshipCurve };

			generateMultipleSprites(basePosition, -20.208f, -1.f, -15.f, 10, negOffset);
			generateMultipleSprites(basePosition, -19.792f, -1.f, -15.f, 10, negOffset);
			generateMultipleSprites(basePosition, -20.f, -1.f, -15.208f, 10, negOffset);
			generateMultipleSprites(basePosition, -20.f, -1.f, -14.792f, 10, negOffset);

			return translationMatrix * originToTranslation * xRotationMatrix * translationToOrigin * model2World;
			};

		Mat44f spaceship2World = updateSpaceshipWorldMatrix();

		// matrix
		Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2World)));

		auto updateCameraMovement = [&](float dt) {
			float sinPhi = sin(state.camControl.phi);
			float cosPhi = cos(state.camControl.phi);
			Vec3f movementVec = state.camControl.movementVec;

			if (state.camControl.actionMoveForward) {
				movementVec.x -= kMovementPerSecond_ * dt * sinPhi;
				movementVec.z += kMovementPerSecond_ * dt * cosPhi;
			}
			if (state.camControl.actionMoveBackward) {
				movementVec.x += kMovementPerSecond_ * dt * sinPhi;
				movementVec.z -= kMovementPerSecond_ * dt * cosPhi;
			}
			if (state.camControl.actionMoveLeft) {
				movementVec.x += kMovementPerSecond_ * dt * cosPhi;
				movementVec.z += kMovementPerSecond_ * dt * sinPhi;
			}
			if (state.camControl.actionMoveRight) {
				movementVec.x -= kMovementPerSecond_ * dt * cosPhi;
				movementVec.z -= kMovementPerSecond_ * dt * sinPhi;
			}
			if (state.camControl.actionMoveUp) {
				movementVec -= kMovementPerSecond_ * dt * Vec3f{ 0.f, 1.f, 0.f };
			}
			if (state.camControl.actionMoveDown) {
				movementVec += kMovementPerSecond_ * dt * Vec3f{ 0.f, 1.f, 0.f };
			}

			state.camControl.movementVec = movementVec;
			};
		updateCameraMovement(dt);

		Mat44f Rx = make_rotation_x(state.camControl.theta);
		Mat44f Ry = make_rotation_y(state.camControl.phi);
		Mat44f T = make_translation(state.camControl.movementVec);

		Mat44f world2Camera = Rx * Ry * T;

		Mat44f projection = make_perspective_projection(
			60 * kPi_ / 180.f,
			fbwidth / float(fbheight),
			0.1f,
			100.f);

		Mat44f projCameraWorld = projection * (world2Camera * model2World);
		Mat44f spaceshipModel2World = projection * (world2Camera * spaceship2World);
		updateSprites(dt);
		updateSpritePositions(sprites);


		// Draw scene
		OGL_CHECKPOINT_DEBUG();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderSprites(projCameraWorld, prog3.programId());
	
		glUseProgram(prog.programId());

		glUniformMatrix4fv(
			0,
			1,
			GL_TRUE,
			projCameraWorld.v
		);

		glUniformMatrix3fv(
			1,
			1,
			GL_TRUE,
			normalMatrix.v
		);

		Vec3f lightDir = normalize(Vec3f{ 0.f, 1.f, -1.f });
		glUniform3fv(2, 1, &lightDir.x);     
		glUniform3f(3, 0.9f, 0.9f, 0.9f);   
		glUniform3f(4, 0.05f, 0.05f, 0.05f); 

		glBindVertexArray(vao);

		if (textures != 0) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures);
		}
		else {
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glDrawArrays(GL_TRIANGLES, 0, vertexCount);

		glUseProgram(prog2.programId());

		auto setUniformMatrix4fv = [](GLuint location, const Mat44f& matrix) {
			glUniformMatrix4fv(location, 1, GL_TRUE, matrix.v);
			};
		setUniformMatrix4fv(0, projCameraWorld);

		auto setUniformMatrix3fv = [](GLuint location, const Mat33f& matrix) {
			glUniformMatrix3fv(location, 1, GL_TRUE, matrix.v);
			};
		setUniformMatrix3fv(1, normalMatrix);

		auto setLightingUniforms = []() {
			Vec3f lightDirection = normalize(Vec3f{ 0.f, 1.f, -1.f });
			glUniform3fv(2, 1, &lightDirection.x);
			glUniform3f(3, 0.9f, 0.9f, 0.9f);
			glUniform3f(4, 0.05f, 0.05f, 0.05f);
			};
		setLightingUniforms();

		auto bindTexture = [](GLuint textureId) {
			if (textureId != 0) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, textureId);
			}
			else {
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			};
		bindTexture(0);

		auto executeDrawCall = [](GLuint vaoId, size_t count) {
			glBindVertexArray(vaoId);
			glDrawArrays(GL_TRIANGLES, 0, count);
			};
		executeDrawCall(launch_vao_1, launchVertexCount);

		// Draw the second launchpad
		glUseProgram(prog2.programId());

	
		auto uploadMatrix4 = [](GLuint location, const Mat44f& matrix) {
			glUniformMatrix4fv(location, 1, GL_TRUE, matrix.v);
			};
		uploadMatrix4(0, projCameraWorld);

	
		auto uploadMatrix3 = [](GLuint location, const Mat33f& matrix) {
			glUniformMatrix3fv(location, 1, GL_TRUE, matrix.v);
			};
		uploadMatrix3(1, normalMatrix);

	
		auto configureLighting = []() {
			auto calculateLightDirection = []() -> Vec3f {
				return normalize(Vec3f{ 0.f, 1.f, -1.f });
				};
			Vec3f lightDir = calculateLightDirection();

			glUniform3fv(2, 1, &lightDir.x); 
			glUniform3f(3, 0.9f, 0.9f, 0.9f); 
			glUniform3f(4, 0.05f, 0.05f, 0.05f); 
			};
		configureLighting();


		auto handleTextureBinding = [](GLuint textureId) {
			if (textureId != 0) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, textureId);
			}
			else {
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			};
		handleTextureBinding(0);


		auto drawMesh = [](GLuint vao, size_t count) {
			auto bindVAO = [](GLuint vaoId) {
				glBindVertexArray(vaoId);
				};
			bindVAO(vao);
			glDrawArrays(GL_TRIANGLES, 0, count);
			};
		drawMesh(launch_vao_2, launchVertexCount);

		// Draw ship
		auto mesh_renderer = [](GLuint vao, size_t vertexCount, GLuint textureObjectId, GLuint programID, Mat44f projCameraWorld, Mat33f normalMatrix) {
			auto setUniformMatrix4fv = [](GLuint location, const Mat44f& matrix) {
				glUniformMatrix4fv(location, 1, GL_TRUE, matrix.v);
				};

			auto setUniformMatrix3fv = [](GLuint location, const Mat33f& matrix) {
				glUniformMatrix3fv(location, 1, GL_TRUE, matrix.v);
				};

			auto setLightingUniforms = []() {
				Vec3f lightDirection = normalize(Vec3f{ 0.f, 1.f, -1.f });
				glUniform3fv(2, 1, &lightDirection.x);
				glUniform3f(3, 0.9f, 0.9f, 0.9f);    
				glUniform3f(4, 0.05f, 0.05f, 0.05f);
				};

			auto bindTexture = [](GLuint textureId) {
				if (textureId != 0) {
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, textureId);
				}
				else {
					glBindTexture(GL_TEXTURE_2D, 0);
				}
				};

			auto executeDrawCall = [](GLuint vaoId, GLsizei count) {
				glBindVertexArray(vaoId);
				glDrawArrays(GL_TRIANGLES, 0, count);
				};

			glUseProgram(programID);

			setUniformMatrix4fv(0, projCameraWorld);
			setUniformMatrix3fv(1, normalMatrix);
			setLightingUniforms();
			bindTexture(textureObjectId);

			// Convert vertexCount to GLsizei to avoid warnings
			GLsizei vertexCountGL = static_cast<GLsizei>(vertexCount);
			executeDrawCall(vao, vertexCountGL);
			};
		mesh_renderer(ship_one_vao, shipVertexCount, 0, prog2.programId(), spaceshipModel2World, normalMatrix);

		
		glBindVertexArray(0);
		glUseProgram(0);

		OGL_CHECKPOINT_DEBUG();
		glfwSwapBuffers( window );
		glfwPollEvents();
	}

	// Cleanup.
	//TODO: additional cleanup
	glDeleteVertexArrays(1, &VAO); 
	glDeleteBuffers(1, &VBO); 
	glDeleteProgram(prog3.programId()); 

	glfwTerminate();
	
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

	void glfw_callback_key_(GLFWwindow* aWindow, int aKey, int, int aAction, int)
	{
		if (GLFW_KEY_ESCAPE == aKey && GLFW_PRESS == aAction)
		{
			glfwSetWindowShouldClose(aWindow, GLFW_TRUE);
			return;
		}

		if (auto* st = static_cast<State_*>(glfwGetWindowUserPointer(aWindow))) {
			// Shader reload functionality
			if (GLFW_KEY_R == aKey && GLFW_PRESS == aAction) {
				if (st->prog) {
					try {
						st->prog->reload();
						std::fprintf(stderr, "Shaders reloaded and recompiled.\n");
					}
					catch (std::exception const& ex) {
						std::fprintf(stderr, "Error when reloading shader:\n");
						std::fprintf(stderr, "%s\n", ex.what());
						std::fprintf(stderr, "Keeping old shader.\n");
					}
				}
			}

			// Movement speed modification (SHIFT)
			if (GLFW_KEY_LEFT_SHIFT == aKey) {
				if (GLFW_PRESS == aAction) {
					kMovementPerSecond_ *= 10.f;
				}
				else if (GLFW_RELEASE == aAction) {
					kMovementPerSecond_ /= 10.f;
				}
			}

			// Movement speed modification (CTRL)
			if (GLFW_KEY_LEFT_CONTROL == aKey) {
				if (GLFW_PRESS == aAction) {
					kMovementPerSecond_ /= 2.f;
				}
				else if (GLFW_RELEASE == aAction) {
					kMovementPerSecond_ *= 2.f;
				}
			}

			// Camera toggling with SPACE
			if (GLFW_KEY_SPACE == aKey && GLFW_PRESS == aAction) {
				st->camControl.cameraActive = !st->camControl.cameraActive;
				if (st->camControl.cameraActive) {
					glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
				}
				else {
					glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
			}

			// Camera movement controls
			if (st->camControl.cameraActive) {
				bool press = GLFW_PRESS == aAction;
				bool release = GLFW_RELEASE == aAction;

				if (GLFW_KEY_W == aKey) st->camControl.actionMoveForward = press ? true : (release ? false : st->camControl.actionMoveForward);
				if (GLFW_KEY_S == aKey) st->camControl.actionMoveBackward = press ? true : (release ? false : st->camControl.actionMoveBackward);
				if (GLFW_KEY_A == aKey) st->camControl.actionMoveLeft = press ? true : (release ? false : st->camControl.actionMoveLeft);
				if (GLFW_KEY_D == aKey) st->camControl.actionMoveRight = press ? true : (release ? false : st->camControl.actionMoveRight);
				if (GLFW_KEY_Q == aKey) st->camControl.actionMoveDown = press ? true : (release ? false : st->camControl.actionMoveDown);
				if (GLFW_KEY_E == aKey) st->camControl.actionMoveUp = press ? true : (release ? false : st->camControl.actionMoveUp);

				if (GLFW_KEY_RIGHT_SHIFT == aKey) kMovementPerSecond_ *= (press ? 2.f : (release ? 0.5f : 1.f));
				if (GLFW_KEY_RIGHT_CONTROL == aKey) kMovementPerSecond_ *= (press ? 0.5f : (release ? 2.f : 1.f));
			}

			// Spaceship animation control
			if (GLFW_KEY_F == aKey) {
				st->moveUp = true;
			}
			else if (GLFW_KEY_R == aKey) {
				st->moveUp = false;
				st->spaceshipOrigin = 0.f;
				st->spaceshipCurve = 0.f;
				st->curve = 0.005f;
				st->acceleration = 0.1f;
			}
		}
	}

	void glfw_callback_motion_(GLFWwindow* wnd, double xPos, double yPos) {
	    if (auto* st = static_cast<State_*>(glfwGetWindowUserPointer(wnd))) {
	        if (st->camControl.cameraActive) {
	            // Calculate delta movement for mouse
	            float deltaX = float(xPos - st->camControl.lastX);
	            float deltaY = float(yPos - st->camControl.lastY);

	            // Update phi for horizontal rotation
	            st->camControl.phi += deltaX * kMouseSensitivity_;

	            // Update theta for vertical rotation with clamping
	            float newTheta = st->camControl.theta + deltaY * kMouseSensitivity_;
	            if (newTheta > kPi_ / 2.f) {
	                st->camControl.theta = kPi_ / 2.f;
	            } else if (newTheta < -kPi_ / 2.f) {
	                st->camControl.theta = -kPi_ / 2.f;
	            } else {
	                st->camControl.theta = newTheta;
	            }
	        }

	        // Update last mouse positions
	        st->camControl.lastX = float(xPos);
	        st->camControl.lastY = float(yPos);
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

//different use program 

