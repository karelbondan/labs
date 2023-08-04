// include C++ headers
#include <cstdio>
#include <iostream>
#include <string>
//using namespace std;	// to avoid having to use std::

// include OpenGL related headers
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>

// global variables
// settings
unsigned int gWindowWidth = 800;
unsigned int gWindowHeight = 600;

// function initialise scene and render settings
static void init(GLFWwindow* window)
{
	// set the color the color buffer should be cleared to
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
}

// function to render the scene
static void render_scene()
{
	// clear color buffer
	glClear(GL_COLOR_BUFFER_BIT);

	// flush the graphics pipeline
	glFlush();
}

// error callback function
static void error_callback(int error, const char* description)
{
	std::cerr << description << std::endl;	// output error description
}

// input handler function
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// close the window when the ESCAPE key is pressed
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		// set flag to close the window
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}

	if (key == GLFW_KEY_Q) {
		glClearColor(1.0f, 0.2f, 0.2f, 1.0f);
	}
	
	if (key == GLFW_KEY_W) {
		glClearColor(1.0f, 1.0f, 0.2f, 1.0f);
	}

	if (key == GLFW_KEY_E) {
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

// pointer cursor event handler
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// quit if the right mouse button is pressed
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		// set flag to close the window
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}
	
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		// get cursor position and log it to the console 
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		std::cout << "x:" << xpos << " y:" << ypos << std::endl;
	}
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	//std::cout << width << " " << height << std::endl;
	const std::string window_size= std::to_string(width) + "x" + std::to_string(height);
	glfwSetWindowTitle(window, window_size.c_str());
}

int main(void)
{
	GLFWwindow* window = nullptr;	// GLFW window handle

	glfwSetErrorCallback(error_callback);	// set GLFW error callback function

	// initialise GLFW
	if (!glfwInit())
	{
		// if failed to initialise GLFW
		exit(EXIT_FAILURE);
	}

	// minimum OpenGL version 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// create a window and its OpenGL context
	window = glfwCreateWindow(gWindowWidth, gWindowHeight, "Lab 1", nullptr, nullptr);

	// set handler for key inputs to glfwSetKeyCallback
	glfwSetKeyCallback(window, key_callback);

	// listen for mouse pointer events
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// check if window created successfully
	if (window == nullptr)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);	// set window context as the current context
	glfwSwapInterval(1);			// swap buffer interval

	// initialise GLEW
	if (glewInit() != GLEW_OK)
	{
		// if failed to initialise GLEW
		std::cerr << "GLEW initialisation failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	// initialise scene and render settings
	init(window);

	int random_number_because_why_not = 1945;
	std::string str = "hehe indonesian independence day hehe " + std::to_string(random_number_because_why_not);
	glfwSetWindowTitle(window, str.c_str());

	glfwSetWindowSizeCallback(window, framebuffer_size_callback);

	// the rendering loop
	while (!glfwWindowShouldClose(window))
	{
		render_scene();		// render the scene

		// get resolution of the window
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		glfwSwapBuffers(window);	// swap buffers
		glfwPollEvents();			// poll for events
	}

	// close the window and terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}