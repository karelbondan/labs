/*
Foreword: I am using the lab 1 project solution, hence the name of solution itself and the folders in 
the solution explorer. I've tried initializing a new project myself by following the instruction in 
the PDF given in Moodle, but it failed although I've followed the exact steps.
*/

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include "ShaderProgram.h"

// initial window height and width. won't be changed anymore so i made 
// it const. 
const int window_width = 1024;
const int window_height = 768;
struct VertexColor {
	GLfloat position[3]; // 3 indexes in the vector are pos 
	GLfloat color[3]; // 3 indexes in the vector are colors
};
const std::vector<float> colors[5] = { 
	{0.2f, 0.2f, 0.2f, 1.0f}, // grey
	{1.0f, 1.0f, 1.0f, 1.0f}, // white 
	{1.0f, 0.816f, 0.0f, 1.0f}, // yellow
	{0.078f, 0.89f, 0.949, 1.0f}, // cyan
	{0.6f, 0.0f, 0.518f, 1.0f} // magenta
};
int col_idx = 0; // current color index variable


GLint polygon_mode; // current polygon render mode
ShaderProgram shader; // shader program obj
GLuint gVBO = 0; // vertex buffer obj identifier
GLuint gVAO = 0; // vertex array obj identifier

// TODO: CHANGE EACH OBJECT COLORS. 

// function to initialize scene and render settings
static void init() {
	// set bg color
	glClearColor(colors[0][0], colors[0][1], colors[0][2], colors[0][3]);
	
	// compile and link vertices and fragment shader pair
	shader.compileAndLink("color.vert", "color.frag");

	std::vector<GLfloat> vertices = {
		// points
		-0.9f, -0.9f, 0.0f, 1.0f, 0.0f, 0.0f,
		-0.8f, -0.9f, 0.0f, 1.0f, 0.0f, 0.0f,
		-0.7f, -0.9f, 0.0f, 1.0f, 0.0f, 0.0f,
		-0.6f, -0.9f, 0.0f, 1.0f, 0.0f, 0.0f,
		-0.5f, -0.9f, 0.0f, 1.0f, 0.0f, 0.0f,
		-0.4f, -0.9f, 0.0f, 1.0f, 0.0f, 0.0f,

		// lines
		0.6f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 0.3f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 0.13f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.4f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		1.0f, -0.05f, 0.0f, 1.0f, 0.0f, 0.0f,

		// triangles
		-0.9f, 0.9f, 0.0f, 1.0f, 0.0f, 0.0f,
		-0.65f, 0.9f, 0.0f, 1.0f, 0.0f, 0.0f,
		-0.9f, 0.65f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.9f, -0.9f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.65f, -0.9f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.9f, -0.65f, 0.0f, 1.0f, 0.0f, 0.0f,

		// triangle strip 
		-0.3f, 0.3f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.3f, 0.3f, 0.0f, 1.0f, 0.0f, 0.0f,
		-0.3f, -0.3f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.3f, -0.3f, 0.0f, 1.0f, 0.0f, 0.0f,

		
	};
	// create and bind (buffer) the vbo.
	glGenBuffers(1, &gVBO);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	glGenVertexArrays(1, &gVAO);
	glBindVertexArray(gVAO);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor), reinterpret_cast<void*>(offsetof(VertexColor, position)));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor), reinterpret_cast<void*>(offsetof(VertexColor, color)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

}

// function to render the scene
static void renderScene() {
	// clear the color buffer first then flush the graphics pipeline
	glClear(GL_COLOR_BUFFER_BIT);

	shader.use();

	glBindVertexArray(gVAO);
	glDrawArrays(GL_POINTS, 0, 6);
	glPointSize(10);
	glDrawArrays(GL_LINES, 6, 6);
	glDrawArrays(GL_TRIANGLES, 12, 6);
	glDrawArrays(GL_TRIANGLE_STRIP, 18, 4);

	glFlush();
}

// error callback to handle errors
static void errorCallback(int error, const char* description) {
	// output error description
	std::cerr << description << std::endl;
}

// keyboard input listener callback 
static void keyCallback(GLFWwindow* window, int key, int scan_code, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		// tell opengl that the scene is window is about to be closed
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}
	else if (key == GLFW_KEY_B && action == GLFW_PRESS) {
		col_idx++; // increment the col_idx first 
		if (col_idx > 4) // if greater than 4 then reset the index
			col_idx = 0;

		// set the bg color
		glClearColor(colors[col_idx][0], colors[col_idx][1], colors[col_idx][2], colors[col_idx][3]);
	}
	else if (key == GLFW_KEY_W && action == GLFW_PRESS) {
		// get the current render mode
		glGetIntegerv(GL_POLYGON_MODE, &polygon_mode);
		// if current render mode is fill then switch to wireframe, vice versa. 
		if (polygon_mode == GL_FILL) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}
}

// mouse input listener callback
static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		// use the glfwgetcursor method to get the position.
		// since the method only accepts double type then we need
		// to declare double variables to store the x and y coords 
		// of the cursor. 
		double x, y;
		glfwGetCursorPos(window, &x, &y);

		// print the coords to the console.
		std::cout << "x:" << x << " y:" << y << std::endl;
	}
}

// window size change listener
static void frameBufferSizeCallback(GLFWwindow* window, int width, int height) {
	// set window title to the new window size.
	glfwSetWindowTitle(window, std::string("Lab Submission 8331030 - " + std::to_string(width) + "x" + std::to_string(height)).c_str());
}

int main(void) {
	GLFWwindow* window = nullptr; // the window variable to store everything

	glfwSetErrorCallback(errorCallback); // setting error callback.

	// if not properly initialized then exit with failure
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	// set the opengl min version to 3.3, set the forward compability 
	// to true (???), and set the current version to the current core 
	// version. 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// bind window to the system 
	window = glfwCreateWindow(window_width, window_height, std::string("Lab Submission 8331030 - " + std::to_string(window_width) + " x " + std::to_string(window_height)).c_str(), nullptr, nullptr);
	glfwSetKeyCallback(window, keyCallback); // listen keyboard inputs
	glfwSetMouseButtonCallback(window, mouseCallback); // listen mouse inputs
	
	// if window fails to initialize then exit
	if (window == nullptr) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); // set context to prev init-ed window object.. 
	glfwSwapInterval(1); // swap buffer interval, im assuming it's 1ms. 

	// set up glew, if fail exit
	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW initialization failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	// i removed the window parameter because i don't see the usage of it 
	// inside the function.
	init();  // init the window (set the bg color to light gray)

	// set buffer size change callback
	glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);

	// main loop
	while (!glfwWindowShouldClose(window)) {
		renderScene(); // render scene

		glfwSwapBuffers(window); // swap the buffer every frame update
		glfwPollEvents(); // poll for events;
	}

	glDeleteBuffers(1, &gVBO);
	glDeleteVertexArrays(1, &gVAO);

	glfwDestroyWindow(window); // destroy window
	glfwTerminate(); // terminate and delete object from memory (?)

	exit(EXIT_SUCCESS); // successful exit without any error
}
