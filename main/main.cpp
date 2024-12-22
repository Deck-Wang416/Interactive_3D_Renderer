#include "spaceship.hpp"

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
		{ GL_VERTEX_SHADER, "C:\\Users\\32939\\Desktop\\houge\\assets\\default.vert" },
		{ GL_FRAGMENT_SHADER, "C:\\Users\\32939\\Desktop\\houge\\assets\\default.frag" }
		});

	state.prog = &prog;
	state.camControl.radius = 10.f;

	auto last = Clock::now();
	float angle = 0.f;

	//-------------------------------------------------------------------


	// Load the map OBJ file
	auto parlahti = load_wavefront_obj("C:\\Users\\32939\\Desktop\\houge\\assets\\parlahti.obj");
	
	GLuint vao = create_vao(parlahti);
	std::size_t vertexCount = parlahti.positions.size();


	GLuint textures = load_texture_2d("C:\\Users\\32939\\Desktop\\houge\\assets\\L4343A-4k.jpeg");

	//----------------------------------------------------------------
	// Load shader program for launchpad
	ShaderProgram prog2 = ShaderProgram({
		{GL_VERTEX_SHADER, "C:\\Users\\32939\\Desktop\\houge\\assets\\launch.vert"},
		{GL_FRAGMENT_SHADER, "C:\\Users\\32939\\Desktop\\houge\\assets\\launch.frag"}
		});

	// Load the launchpad
	SimpleMeshData launch = load_wavefront_obj("C:\\Users\\32939\\Desktop\\houge\\assets\\landingpad.obj");
	std::size_t launchVertexCount = launch.positions.size();
	std::vector<Vec3f> originalPositions(launch.positions.begin(), launch.positions.end());

	// Helper function to move positions
	auto adjustLaunchPositions = [&](SimpleMeshData& mesh, Vec3f offset) {
		for (Vec3f& position : mesh.positions) {
			position = position + offset;
		}
		};

	// Move the first launch object
	adjustLaunchPositions(launch, Vec3f{ 0.f, -0.975f, -50.f });

	// Create VAO for the first launchpad
	GLuint launch_vao_1 = create_vao(launch);

	// Reset positions
	launch.positions.assign(originalPositions.begin(), originalPositions.end());

	// Move the second launch object
	adjustLaunchPositions(launch, Vec3f{ -20.f, -0.975f, -15.f });

	// Create VAO for the second launchpad
	GLuint launch_vao_2 = create_vao(launch);

	 // SHIP CREATION SECTION
	//-------------------------------------------------------------------

	 // Create the spaceship
	 auto ship = spaceship();
	 size_t shipVertexCount = ship.positions.size();


	 // Move the ship
	 for (size_t i = 0; i < shipVertexCount; i++)
	 {
		 ship.positions[i] = ship.positions[i] + Vec3f{ -20.f, -1.125f, -15.f };
	 }

	 // Create VAO for the ship
	 GLuint ship_one_vao = create_vao(ship);

	 //-------------------------------------------------------------------
	 // SHIP CREATION SECTION END

	//-------------------------------------------------------------------
	// POINT SPRITE CREATION
	// loadTexture();
	 //setupSpriteBuffers();
	 ShaderProgram prog3({
			 { GL_VERTEX_SHADER, "C:\\Users\\32939\\Desktop\\houge\\assets\\points.vert" },
			 { GL_FRAGMENT_SHADER, "C:\\Users\\32939\\Desktop\\houge\\assets\\points.frag" }
	 });
	 loadTexture();
	 setupSpriteBuffers();
	 
	// POINT SPRITE CREATION END
	//-------------------------------------------------------------------


	// Other initialization & loading
	OGL_CHECKPOINT_ALWAYS();

	// Main loop
	while( !glfwWindowShouldClose( window ) )
	{
		// Let GLFW process events
		glfwPollEvents();

		//generateSprites(Vec3f{ 1.f, 1.f, 1.f }, 10);
		
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
		// Time calculations
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

		// Spaceship animation
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

			// Sprite generation helper
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

		// Normal matrix
		Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2World)));

		// Camera transformations
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

		// Sprite updates
		updateSprites(dt);
		updateSpritePositions(sprites);

		//ENDOF TODO

		// Draw scene
		OGL_CHECKPOINT_DEBUG();

		//TODO: draw frame
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderSprites(projCameraWorld, prog3.programId());
	
		// Draw the map
		glUseProgram(prog.programId());

		// Set projection-camera-world matrix
		glUniformMatrix4fv(
			0,
			1,
			GL_TRUE,
			projCameraWorld.v
		);

		// Set normal matrix
		glUniformMatrix3fv(
			1,
			1,
			GL_TRUE,
			normalMatrix.v
		);

		// Set lighting uniforms
		Vec3f lightDir = normalize(Vec3f{ 0.f, 1.f, -1.f });
		glUniform3fv(2, 1, &lightDir.x);      // Ambient light direction
		glUniform3f(3, 0.9f, 0.9f, 0.9f);    // Diffusion
		glUniform3f(4, 0.05f, 0.05f, 0.05f); // Spectral highlights

		// Bind vertex array object
		glBindVertexArray(vao);

		// Bind texture if available
		if (textures != 0) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures);
		}
		else {
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// Issue draw call
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);

		// Draw the first launchpad
		glUseProgram(prog2.programId());

		// Set projection-camera-world matrix
		auto setUniformMatrix4fv = [](GLuint location, const Mat44f& matrix) {
			glUniformMatrix4fv(location, 1, GL_TRUE, matrix.v);
			};
		setUniformMatrix4fv(0, projCameraWorld);

		// Set normal matrix
		auto setUniformMatrix3fv = [](GLuint location, const Mat33f& matrix) {
			glUniformMatrix3fv(location, 1, GL_TRUE, matrix.v);
			};
		setUniformMatrix3fv(1, normalMatrix);

		// Set lighting uniforms
		auto setLightingUniforms = []() {
			Vec3f lightDirection = normalize(Vec3f{ 0.f, 1.f, -1.f });
			glUniform3fv(2, 1, &lightDirection.x); // Ambient light direction
			glUniform3f(3, 0.9f, 0.9f, 0.9f);     // Diffusion
			glUniform3f(4, 0.05f, 0.05f, 0.05f);  // Specular
			};
		setLightingUniforms();

		// Bind texture
		auto bindTexture = [](GLuint textureId) {
			if (textureId != 0) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, textureId);
			}
			else {
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			};
		bindTexture(0); // Since textureObjectId is 0, bind no texture

		// Issue draw call
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
				glUniform3fv(2, 1, &lightDirection.x); // Ambient light direction
				glUniform3f(3, 0.9f, 0.9f, 0.9f);     // Diffusion
				glUniform3f(4, 0.05f, 0.05f, 0.05f);  // Specular
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

		
		
		//renderSprites(projCameraWorld, prog3.programId());

		glBindVertexArray(0);
		//glBindVertexArray(1);

		glUseProgram(0);
		//glUseProgram(1);

		//ENDOF TODO

		OGL_CHECKPOINT_DEBUG();

		// Display results
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

		if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow)))
		{
			// R-key reloads shaders.
			if (GLFW_KEY_R == aKey && GLFW_PRESS == aAction)
			{
				if (state->prog)
				{
					try
					{
						state->prog->reload();
						std::fprintf(stderr, "Shaders reloaded and recompiled.\n");
					}
					catch (std::exception const& eErr)
					{
						std::fprintf(stderr, "Error when reloading shader:\n");
						std::fprintf(stderr, "%s\n", eErr.what());
						std::fprintf(stderr, "Keeping old shader.\n");
					}
				}
			}

			// TODO SHIFT INCREASES SPEED

			if (GLFW_KEY_LEFT_SHIFT == aKey && GLFW_PRESS == aAction)
				kMovementPerSecond_ *= 10.f;
			else if (GLFW_KEY_LEFT_SHIFT == aKey && GLFW_RELEASE == aAction)
				kMovementPerSecond_ /= 10.f;


			// TODO CTRL DECREASES SPEED
			if (GLFW_KEY_LEFT_CONTROL == aKey && GLFW_PRESS == aAction) 
				kMovementPerSecond_ /= 2.f;
			else if (GLFW_KEY_LEFT_CONTROL == aKey && GLFW_RELEASE == aAction)
				kMovementPerSecond_ *= 2.f;


			// Space toggles camera
			if (GLFW_KEY_SPACE == aKey && GLFW_PRESS == aAction)
			{
				state->camControl.cameraActive = !state->camControl.cameraActive;

				if (state->camControl.cameraActive)
					glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
				else
					glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}

			// Camera controls if camera is active
			if (state->camControl.cameraActive)
			{
				if (GLFW_KEY_W == aKey)
				{
					if (GLFW_PRESS == aAction)
						state->camControl.actionMoveForward = true;
					else if (GLFW_RELEASE == aAction)
						state->camControl.actionMoveForward = false;
				}
				else if (GLFW_KEY_S == aKey)
				{
					if (GLFW_PRESS == aAction)
						state->camControl.actionMoveBackward = true;
					else if (GLFW_RELEASE == aAction)
						state->camControl.actionMoveBackward = false;
				}
				else if (GLFW_KEY_A == aKey)
				{
					if (GLFW_PRESS == aAction)
						state->camControl.actionMoveLeft = true;
					else if (GLFW_RELEASE == aAction)
						state->camControl.actionMoveLeft = false;
				}
				else if (GLFW_KEY_D == aKey)
				{
					if (GLFW_PRESS == aAction)
						state->camControl.actionMoveRight = true;
					else if (GLFW_RELEASE == aAction)
						state->camControl.actionMoveRight = false;
				}
				else if (GLFW_KEY_Q == aKey)
				{
					if (GLFW_PRESS == aAction)
						state->camControl.actionMoveDown = true;
					else if (GLFW_RELEASE == aAction)
						state->camControl.actionMoveDown = false;
				}
				else if (GLFW_KEY_E == aKey)
				{
					if (GLFW_PRESS == aAction)
						state->camControl.actionMoveUp = true;
					else if (GLFW_RELEASE == aAction)
						state->camControl.actionMoveUp = false;
				}
				else if (GLFW_KEY_RIGHT_SHIFT == aKey)
				{
					if (GLFW_PRESS == aAction)
						kMovementPerSecond_ *= 2.f;
					else if (GLFW_RELEASE == aAction)
						kMovementPerSecond_ /= 2.f;
				}
				else if (GLFW_KEY_RIGHT_CONTROL == aKey)
				{
					if (GLFW_PRESS == aAction)
						kMovementPerSecond_ /= 2.f;
					else if (GLFW_RELEASE == aAction)
						kMovementPerSecond_ *= 2.f;
				}
			}

			// Spaceship animation control
			if (GLFW_KEY_F == aKey) {
				state->moveUp = true;
			} else if (GLFW_KEY_R == aKey) {
				state->moveUp = false;
				state->spaceshipOrigin = 0.f;
				state->spaceshipCurve = 0.f;
				state->curve = 0.005f;
				state->acceleration = 0.1f;
			}
		}
	}

	void glfw_callback_motion_(GLFWwindow* aWindow, double aX, double aY)
	{
		if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow)))
		{
			if (state->camControl.cameraActive)
			{
				auto const dx = float(aX - state->camControl.lastX);
				auto const dy = float(aY - state->camControl.lastY);

				state->camControl.phi += dx * kMouseSensitivity_;

				state->camControl.theta += dy * kMouseSensitivity_;
				if (state->camControl.theta > kPi_ / 2.f)
					state->camControl.theta = kPi_ / 2.f;
				else if (state->camControl.theta < -kPi_ / 2.f)
					state->camControl.theta = -kPi_ / 2.f;
			}

			state->camControl.lastX = float(aX);
			state->camControl.lastY = float(aY);
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

