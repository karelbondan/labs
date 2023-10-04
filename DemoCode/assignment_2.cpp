// include C++ headers
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <AntTweakBar.h>

// include OpenGL related headers
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>	// include GLM (ideally should only use the GLM headers that are actually used)
#include <glm/gtx/transform.hpp>

#include "ShaderProgram.h"

struct VertexColor
{
	GLfloat position[3];
	GLfloat color[3];
};

// global variables
// settings
unsigned int gWindowWidth = 1366;
unsigned int gWindowHeight = 768;

// scene content
ShaderProgram gShader;	// shader program object
GLuint gIBO = 0;		// index buffer object identifier
GLuint gVBO = 0;		// vertex buffer object identifier
GLuint gVAO = 0;		// vertex array object identifier

unsigned int gSlices = 32;	// number of slices for wheel and hubcap
float gFrameRate = 60.0f;
float gFrameTime = 1 / gFrameRate;
float gScaleFactor = static_cast<float>(gWindowHeight) / gWindowWidth;	// for truck so it's not stretched

// dynamic variables 
float dumpBoxRotation = 0.0f;
float dumpBoxRotationSensitivity = 7.0f;
float moveSpeed = 0.2f;
bool gWireframe = false; // wireframe mode on or off
glm::vec3 truckPos = glm::vec3(0.0f, -0.5f, 0.0f);
glm::vec3 gBackgroundColor(0.2f); // variables to set the background color
glm::vec3 gMoveVec(0.0f); // object translation
glm::vec3 scaleVec(gScaleFactor, 1.0f, 1.0f); // scale vector to unstretch truck

// model matrix
std::map<std::string, glm::mat4> gModelMatrix;	// object model matrix

/*
	function to generate vertices for a circle based on a radius and number of slices
	can be used for generating hubcap and wheels at the same time. only need to change the 
	radius and pass the type (either wheel or hubcap) through the "type" param
*/
void generate_circle(const float radius, const unsigned int slices, const float scale_factor, std::vector<GLfloat> &vertices, std::string type)
{
	float slice_angle = M_PI * 2.0f / slices;	// angle of each slice
	float angle = 0;			// angle used to generate x and y coordinates
	float x, y, z = 0.0f;		// (x, y, z) coordinates

	// debug
	std::cout << type << (type == "wheel") << std::endl;
	std::cout << type << (type == "hubcap") << std::endl;

	// generate vertex coordinates for a circle
	for (int i = 0; i <= slices; i++)
	{
		x = radius * cos(angle);
		y = radius * sin(angle);

		// vertices
		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(z);

		// color 
		if (type == "wheel") {
			
			/*
* 				if the current interation is between 14 and 15,
				and 30 and 31, change the outer vertices color  
				to light gray. this will create an illusion of shine
				(make sure to adjust based on the number of slices)
			*/
			if (i == 15 || i == 31 || i == 14 || i == 30) {
				vertices.push_back(0.671f);
				vertices.push_back(0.671f);
				vertices.push_back(0.671f);
			}
			else {
				vertices.push_back(0.471f);
				vertices.push_back(0.471f);
				vertices.push_back(0.471f);
			}
		}
		// if the passed type is hubcap, set the outer vertices
		// to dark gray color
		else if (type == "hubcap") {
			vertices.push_back(0.071f);
			vertices.push_back(0.071f);
			vertices.push_back(0.071f);
		}

		// update to next angle
		angle += slice_angle;
	}
}

// function initialise scene and render settings
static void init(GLFWwindow* window)
{
	// set the color the color buffer should be cleared to
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	// compile and link a vertex and fragment shader pair
	gShader.compileAndLink("simple.vert", "simple.frag");

	// model matrices that'll be used to store info about
	// the location of the vertices. chassis will use IBO;
	// meanwhile the rest will use drawarray
	gModelMatrix["Ground"] = glm::mat4(1.0f);
	gModelMatrix["Chassis"] = glm::mat4(1.0f);
	gModelMatrix["DumpBox"] = glm::mat4(1.0f);
	gModelMatrix["Hubcap1"] = glm::mat4(1.0f);
	gModelMatrix["Wheel1"] = glm::mat4(1.0f);
	gModelMatrix["Hubcap2"] = glm::mat4(1.0f);
	gModelMatrix["Wheel2"] = glm::mat4(1.0f);

	std::vector<GLfloat> vertices = {
		// ground
		-1.0f, -0.7f, 0.0f, 0.251f, 0.812f, 0.455f, // upper left
		1.0f, -0.7f, 0.0f, 0.251f, 0.812f, 0.455f, // upper right
		-1.0f, -1.0f, 0.0f, 0.0f, 0.651f, 0.235f, // lower left
		1.0f, -1.0f, 0.0f, 0.0f, 0.651f, 0.235f, // lower right

		// chassis 
		-0.8f, 0.1f, 0.0f, 0.831f, 0.831f, 0.831f, // upper left
		0.8f, 0.1f, 0.0f, 0.831f, 0.831f, 0.831f, // upper right
		-0.8f, 0.0f, 0.0f, 0.439f, 0.439f, 0.439f, // lower left
		0.8f, 0.0f, 0.0f, 0.439f, 0.439f, 0.439f, // lower left

		// driver 
		-0.8f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // upper left
		-0.25f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, // upper right
		-0.8f, 0.1f, 0.0f, 1.0f, 1.0f, 0.0f, // lower left
		-0.25f, 0.1f, 0.0f, 1.0f, 1.0f, 0.0f, // lower right

		// upper driver
		-0.25f, 0.8f, 0.0f, 1.0f, 0.0f, 0.0f, // upper right -> idx 12
		-0.55f, 0.8f, 0.0f, 1.0f, 1.0f, 1.0f, // upper left
		-0.8f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // lower left
		-0.25f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, // lower right

		// window
		-0.4f, 0.73f, 0.0f, 0.0f, 0.0f, 0.0f, // upper right -> idx 16
		-0.55f, 0.73f, 0.0f, 0.0f, 0.0f, 0.0f, // upper left
		-0.73f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, // lower left
		-0.4f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, // lower right

		// dump box
		0.8f, 0.1f, 0.0f, 1.0f, 0.72f, 0.22f, // lower right -> idx 20
		-0.15f, 0.1f, 0.0f, 1.0f, 0.72f, 0.22f, // lower left 
		0.9f, 0.43f, 0.0f, 1.0f, 0.72f, 0.22f, // middle right
		-0.25f, 0.43f, 0.0f, 1.0f, 0.72f, 0.22f, // middle left
		0.8f, 0.73f, 0.0f, 1.0f, 0.72f, 0.22f, // upper right
		-0.15f, 0.73f, 0.0f, 1.0f, 0.906f, 0.737f, // upper left
	};

	std::vector<GLuint> indices = {
		// chassis
		4, 5, 6,
		6, 5, 7,

		// driver
		10, 11, 9,
		9, 10, 8,

		// upper driver 
		14, 15, 12,
		12, 13, 14,

		// window
		18, 19, 16,
		16, 17, 18
	};

	// hubcap
	vertices.push_back(0.0f);	// x
	vertices.push_back(0.0f);	// y
	vertices.push_back(0.0f);	// z

	vertices.push_back(0.831f);	// r
	vertices.push_back(0.831f);	// g
	vertices.push_back(0.831f);	// b

	generate_circle(0.13, gSlices, gScaleFactor, vertices, "wheel");

	// wheel
	vertices.push_back(0.0f);	// x
	vertices.push_back(0.0f);	// y
	vertices.push_back(0.0f);	// z

	vertices.push_back(0.58f);	// r
	vertices.push_back(0.58f);	// g
	vertices.push_back(0.58f);	// b

	generate_circle(0.2, gSlices, gScaleFactor, vertices, "hubcap");
	
	glGenVertexArrays(1, &gVAO);			// generate unused VAO identifier
	glBindVertexArray(gVAO);				// create VAO

	// create VBO and buffer the data
	glGenBuffers(1, &gVBO);					// generate unused VBO identifier
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);	// bind the VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	// create IBO and buffer the data
	glGenBuffers(1, &gIBO);							// generate unused IBO identifier
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);	// bind the IBO
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), &indices[0], GL_STATIC_DRAW);

	// create VAO, specify VBO data and format of the data
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);	// bind the VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO); // bind ibo 
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor),
		reinterpret_cast<void*>(offsetof(VertexColor, position)));	// specify format of position data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor),
		reinterpret_cast<void*>(offsetof(VertexColor, color)));		// specify format of colour data
	
	glEnableVertexAttribArray(0);	// enable vertex attributes
	glEnableVertexAttribArray(1);
}

// anttweakbar from the 4th lab
TwBar* create_UI(const std::string name) {
	TwBar* twBar = TwNewBar(name.c_str());
	TwWindowSize(gWindowWidth, gWindowHeight);
	TwDefine(" TW_HELP visible=false ");
	TwDefine(" GLOBAL fontsize=3 ");
	TwDefine(" Main label='MyGUI' refresh=0.02 text=light size='220 400' ");

	TwAddVarRO(twBar, " Frame Rate ", TW_TYPE_FLOAT, &gFrameRate,
		"group='Frame Stats' precision=2");
	TwAddVarRO(twBar, "Frame Time", TW_TYPE_FLOAT, &gFrameTime,
		"group='Frame Stats' precision=5");
	TwAddVarRW(twBar, "Wireframe", TW_TYPE_BOOLCPP, &gWireframe,
		"group='Display'");
	TwAddVarRW(twBar, "BgColor", TW_TYPE_COLOR3F, &gBackgroundColor,
		"label='Background Color' group='Display' opened=true");
	TwAddVarRW(twBar, "Angle", TW_TYPE_FLOAT, &dumpBoxRotation,
		"group='Dump Box Control' min=0.0 max=45.0 step=0.05");
	TwAddVarRW(twBar, "Speed", TW_TYPE_FLOAT, &moveSpeed,
		"group='Movement' min=0.1 max=2.0 step=0.005");

	TwAddSeparator(twBar, nullptr, nullptr);

	return twBar;
}


// will be called per frame update
static void update_scene(GLFWwindow* window) {
	/*
		the static keyword is telling cpp it's a "detached" variable, meaning
		it won't reset on the next function call. e.g. in frame 1 the variable is updated
		to 1.0f. in the next call, instead of being reset to 0.0f again, it will still hold
		the value of 1.0f. optionally, this variable can be moved out from here to be declared
		above everything, just like the rest of the global variables. 
	*/
	static float wheelsRotation = 0.0f; 

	// set bg color to gBackgroundColor set using the twbar. the last one is the alpha, 
	// or the transparency. it was set to 1.0f because we don't want any transparent effect.
	glClearColor(gBackgroundColor.r, gBackgroundColor.g, gBackgroundColor.b, 1.0f);

	// wheel
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		// x is subtracted by the move speed to make the truck move left. if addition 
		// is performed it will move to the right instead
		truckPos.x -= moveSpeed * gFrameTime;
		
		// as for the wheel rotation i don't really know how it rotates to the left  
		// when addition is performed; i trial-and-errored this one. 
		wheelsRotation += moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		truckPos.x += moveSpeed * gFrameTime;
		wheelsRotation -= moveSpeed;
	}

	// dump box
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		if (dumpBoxRotation <= 45.0) {
			dumpBoxRotation += dumpBoxRotationSensitivity * gFrameTime;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		if (dumpBoxRotation >= 0.00) {
			dumpBoxRotation -= dumpBoxRotationSensitivity * gFrameTime;
		}
	}

	// transform

	// initially, the truck is suspended in the air and is stretched because of the window size.
	// move the truck 0.5 units down, then scale it based on the gScaleFactor on the x axis 
	// defined above to unstretch it.
	gModelMatrix["Chassis"] = glm::translate(truckPos) * glm::scale(scaleVec);

	/*
		the wheels are defined here.initially, both of them are at position xy 0.
		the transformations of the chassis are applied here by initially multiplying
		gModelMatrix["Chassis"] with the rest of the transformations, i.e. translating 
		0.5 unit to the left, and rotating it based on the wheel rotation
	*/ 
	gModelMatrix["Wheel1"] = gModelMatrix["Chassis"]
		* glm::translate(glm::vec3(-0.5f, 0.0f, 0.0f))
		* glm::rotate(glm::radians(wheelsRotation), glm::vec3(0.0f, 0.0f, 1.0f));

	// for the hubcap, it's simply applying the transformations of the wheel without any 
	// further transformations
	gModelMatrix["Hubcap1"] = gModelMatrix["Wheel1"];

	/*
		the second wheel and hubcap are essentially the same as the first one defined 
		above. the same transformations process are reapplied for them. the only one 
		different being 0.5 point translation to the right. 
	*/
	gModelMatrix["Wheel2"] = gModelMatrix["Chassis"]
		* glm::translate(glm::vec3(0.5f, 0.0f, 0.0f))
		* glm::rotate(glm::radians(wheelsRotation), glm::vec3(0.0f, 0.0f, 1.0f));
	gModelMatrix["Hubcap2"] = gModelMatrix["Wheel2"];
	
	/*
		finally, we initialize the dump box's transformations. we also initially multiply it 
		with chassis'  transformations, then continued by transformations only applied to the dump 
		box. i should mention that the rotation point of every object is firstly relative to the 
		global position of when the object is initialized, meaning that the rotation point 
		is always initially xy 0. this will later then be the objects' own local positioning. 
		in this case, i initially defined the dumpbox 0.1 point up, with the bottom right 
		vertex being 0.8 point to the right from 0.0. from the goal we're trying to achieve, 
		we know we want to rotate the dump box on its bottom right position, i.e. 0.8 point 
		to the right. knowing these two facts, we will first translate the dump box 0.1
		point above and 0.8 point to the right. this will make the rotation point be at 
		x 0.8, y 0.0. after that, we rotate the dumpbox with the angle being negative because 
		we want to rotate it clockwise. after rotating it, we want to translate it back to 
		its original position, i.e. 0.1 point below and 0.8 point to the left. at this stage, 
		the rotation point will also be back to its original position, but it won't matter 
		because we have applied the rotation transformation before the rotation point is 
		translated back to its original position. this will create an illusion that the dump 
		box is rotated on its bottom right corner.
	*/
	gModelMatrix["DumpBox"] = gModelMatrix["Chassis"]
		* glm::translate(glm::vec3(0.8f, 0.1f, 0.0f))
		* glm::rotate(glm::radians(-dumpBoxRotation), glm::vec3(0.0f, 0.0f, 0.1f))
		* glm::translate(glm::vec3(-0.8f, -0.1f, 0.0f));
}

// function to render the scene
static void render_scene()
{
	// clear color buffer
	glClear(GL_COLOR_BUFFER_BIT);
	gShader.use();						// use the shaders associated with the shader program
	glBindVertexArray(gVAO);			// make VAO active

	// chassis and driver compartment
	/*
		as mentioned earlier, we are using indices, which means that 
		we're using IBO, which means that glDrawElements must be 
		used instead of glDrawArray. 26 is the number of vertices
		opengl need to draw. I'm not really sure what the last param 
		does but everyone on SO said just use 0 for it lol. 
	*/
	gShader.setUniform("uModelMatrix", gModelMatrix["Chassis"]);
	glDrawElements(GL_TRIANGLES, 26, GL_UNSIGNED_INT, 0);	

	// ground 
	// start at index 0, draw (use) 4 indices of the "vertices" vector object
	gShader.setUniform("uModelMatrix", gModelMatrix["Ground"]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// dump box 
	// start at index 20, draw (use) 6 indices of the "vertices" vector object
	gShader.setUniform("uModelMatrix", gModelMatrix["DumpBox"]);
	glDrawArrays(GL_TRIANGLE_STRIP, 20, 6);

	// wheels
	/*
		notice that we're reusing the indices from the vertices vector object.
		as long as the object being drawn is the same, we can reuse the indices 
		as many as we want. at least this is what i understand. 
		nb:
		- start at index 60 for the hubcaps, and use 34 indices 
		  (make sure to adjust the number of indices based on the number of slices)
		- start at index 26 for the wheels, and use 34 indices 
		  (make sure to adjust the number of indices based on the number of slices)
	*/
	gShader.setUniform("uModelMatrix", gModelMatrix["Hubcap1"]);
	glDrawArrays(GL_TRIANGLE_FAN, 60, 34);	
	gShader.setUniform("uModelMatrix", gModelMatrix["Wheel1"]);
	glDrawArrays(GL_TRIANGLE_FAN, 26, 34);	
	gShader.setUniform("uModelMatrix", gModelMatrix["Hubcap2"]);
	glDrawArrays(GL_TRIANGLE_FAN, 60, 34);	
	gShader.setUniform("uModelMatrix", gModelMatrix["Wheel2"]);
	glDrawArrays(GL_TRIANGLE_FAN, 26, 34);

	// flush the graphics pipeline
	glFlush();
}

// key press or release callback function
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// close the window when the ESCAPE key is pressed
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		// set flag to close the window
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}
	else if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		// set polygon render mode to wireframe mode
		gWireframe = true;
		return;
	}
	else if (key == GLFW_KEY_S && action == GLFW_PRESS)
	{
		// set polygon render mode to solid mode
		gWireframe = false;
		return;
	}
}

// error callback function
static void error_callback(int error, const char* description)
{
	std::cerr << description << std::endl;	// output error description
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	TwEventMouseButtonGLFW(button, action);
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
	TwEventMousePosGLFW(static_cast<int>(xpos), static_cast<int>(ypos));
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
	window = glfwCreateWindow(gWindowWidth, gWindowHeight, "Assignment 1", nullptr, nullptr);

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

	// set GLFW callback functions
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

	// initialise scene and render settings
	init(window);

	TwInit(TW_OPENGL_CORE, nullptr);
	TwBar* tweakBar = create_UI("Main");

	// timing data
	double lastUpdateTime = glfwGetTime();	// last update time
	double elapsedTime = lastUpdateTime;	// time since last update
	int frameCount = 0;						// number of frames since last update

	// the rendering loop
	while (!glfwWindowShouldClose(window))
	{
		update_scene(window);

		if (gWireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		render_scene();		// render the scene

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		TwDraw();

		glfwSwapBuffers(window);	// swap buffers
		glfwPollEvents();			// poll for events

		frameCount++;
		elapsedTime = glfwGetTime() - lastUpdateTime;	// time since last update

		// if elapsed time since last update > 1 second
		if (elapsedTime > 1.0)
		{
			gFrameTime = elapsedTime / frameCount;	// average time per frame
			gFrameRate = 1 / gFrameTime;			// frames per second
			lastUpdateTime = glfwGetTime();			// set last update time to current time
			frameCount = 0;							// reset frame counter
		}
	}

	// clean up
	glDeleteBuffers(1, &gVBO);
	glDeleteBuffers(1, &gIBO);
	glDeleteVertexArrays(1, &gVAO);

	TwDeleteBar(tweakBar);
	TwTerminate();

	// close the window and terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}