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

		// Spaceship stuff
		bool moveUp = false;
		float spaceshipOrigin = 0.f;
		float spaceshipCurve = 0.f;
		// Slow lift-off
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
