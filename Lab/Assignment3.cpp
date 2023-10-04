#define _USE_MATH_DEFINES
#include <cmath>
#include "utilities.h"
#include "SimpleModel.h"
#include "Camera.h"

#define NUM_OF_MODEL 3
#define NUM_OF_ORBIT 2

// settings
unsigned int gWindowWidth = 1400;
unsigned int gWindowHeight = 768;

// global variables
float aspectRatio = static_cast<float>(gWindowWidth) / gWindowHeight;
int totalSlices = 0;

// frame stats
float gFrameRate = 144.0f;
float gFrameTime = 1 / gFrameRate;

// scene content
ShaderProgram gShader;	

ShaderProgram gShaderOrbitLine;	
GLuint gVBO = 0;		// vertex buffer object identifier
GLuint gVAO = 0;		// vertex array object identifier

std::map<std::string, Material> gMaterials; // holder for objects' selected materials
std::map<int, glm::mat4> gModelMatrix; // model matrix for objects
std::map<int, glm::mat4> gModelMatrixOrbitLine; // model matrix for orbit lines
std::map<int, SimpleModel> gModel; // holder for objects' selected models
std::vector<GLfloat> gVertices; // vertex positions of orbit lines

Light gLight; // light object		

// controls
bool gWireframe = false; 
float gOrbitSpeed[2] = { 0.5f, 1.0f };
float gRotationSpeed[2] = { 1.0f, 1.0f };

Camera gCamera; // virtual camera

// material type
enum class MaterialType { PEARL, JADE, SOMEOTHERMATERIAL };
enum class ModelType { SPHERE, SUZANNE, TORUS, CUBE, CONE, CYLINDER };

std::map<int, ModelType> gModelSelected; // record what material is selected
std::map<int, ModelType> gPreviousModel; // previous selected model; optimization purposes
std::map<int, MaterialType> gMaterialSelected; // record what material is selected

// function to convert enum definitions to string
const char* model_type_to_string(ModelType v)
{
	switch (v)
	{
	case ModelType::SPHERE:		return "sphere";
	case ModelType::SUZANNE:	return "suzanne";
	case ModelType::TORUS:		return "torus";
	case ModelType::CUBE:		return "cube";
	case ModelType::CONE:		return "cone";
	case ModelType::CYLINDER:	return "cylinder";
	default:					return "sphere";
	}
}

// function to load models
static void load_model(int object_index, std::string name) {
	std::string modelPath = "./models/" + name + ".obj";
	gModel[object_index].loadModel(modelPath.c_str());
}

// function to generate orbit line
void generate_circle(const float radius, const unsigned int slices, const float scale_factor, std::vector<GLfloat>& vertices)
{
	for (float i = 0; i < 2 * M_PI; i += 0.01)
	{
		float x = radius * cos(i);
		float y = 0.0f; // rotation axis
		float z = radius * sin(i);

		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(z);

		std::cout << x << " " << y << " " << z << std::endl; // debugging
		totalSlices++;
	}
}

// function initialise scene and render settings
static void init(GLFWwindow* window)
{
	// define materials properties
	gMaterials["Pearl"].Ka = glm::vec3(0.25f, 0.21f, 0.21f);
	gMaterials["Pearl"].Kd = glm::vec3(1.0f, 0.83f, 0.83f);
	gMaterials["Pearl"].Ks = glm::vec3(0.3f, 0.3f, 0.3f);
	gMaterials["Pearl"].shininess = 11.3f;
	
	gMaterials["Jade"].Ka = glm::vec3(0.14f, 0.22f, 0.16f);
	gMaterials["Jade"].Kd = glm::vec3(0.54f, 0.89f, 0.63f);
	gMaterials["Jade"].Ks = glm::vec3(0.32f);
	gMaterials["Jade"].shininess = 12.8f;
	
	gMaterials["SomeOtherMaterial"].Ka = glm::vec3(0.30f, 0.15f, 0.78f);
	gMaterials["SomeOtherMaterial"].Kd = glm::vec3(0.95f, 0.39f, 0.21f);
	gMaterials["SomeOtherMaterial"].Ks = glm::vec3(0.4f);
	gMaterials["SomeOtherMaterial"].shininess = 2.5f;

	// set the color the color buffer should be cleared to
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	glEnable(GL_DEPTH_TEST);	

	gShaderOrbitLine.compileAndLink("simpleColor.vert", "simpleColor.frag");

	gModelMatrixOrbitLine[0] = glm::mat4(1.0f);
	gModelMatrixOrbitLine[1] = glm::mat4(1.0f);
	
	gVertices.clear();

	generate_circle(3.0f, 32, aspectRatio, gVertices);

	glGenBuffers(1, &gVBO);					// generate unused VBO identifier
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);	// bind the VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * gVertices.size(), &gVertices[0], GL_DYNAMIC_DRAW);

	// create VAO, specify VBO data and format of the data
	glGenVertexArrays(1, &gVAO);			// generate unused VAO identifier
	glBindVertexArray(gVAO);				// create VAO
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);	// bind the VBO
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);	// specify format of the data

	glEnableVertexAttribArray(0);	// enable vertex attributes

	// compile and link a vertex and fragment shader pair
	// switch to another shader to be able to render 3d things
	gShader.compileAndLink("lighting.vert", "directionalLight.frag");

	// initialise view matrix
	gCamera.setViewMatrix(glm::vec3(0.0f, 3.0f, 9.0f),
		glm::vec3(0.0f, 0.0f, 0.0f));

	// initialise projection matrix
	gCamera.setProjMatrix(glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 20.0f));

	// initialise point light properties
	gLight.dir = glm::vec3(0.3f, -0.7f, -0.5f);
	gLight.La = glm::vec3(0.8f);
	gLight.Ld = glm::vec3(0.8f);
	gLight.Ls = glm::vec3(0.8f);

	// initialise model matrices
	gModelMatrix[0] = glm::mat4(1.0f);
	gModelMatrix[1] = glm::mat4(1.0f);
	gModelMatrix[2] = glm::mat4(1.0f);

	// load and set initial models properties
	load_model(0, "sphere");
	load_model(1, "suzanne");
	load_model(2, "cube");

	gModelSelected[0] = gPreviousModel[0] = ModelType::SPHERE;
	gModelSelected[1] = gPreviousModel[1] = ModelType::SUZANNE;
	gModelSelected[2] = gPreviousModel[2] = ModelType::CUBE;

	gMaterialSelected[0] = MaterialType::SOMEOTHERMATERIAL;
	gMaterialSelected[1] = MaterialType::JADE;
	gMaterialSelected[2] = MaterialType::PEARL;
}

// function used to update the scene
static void update_scene(GLFWwindow* window)
{
	// static variables for rotation angles
	static float orbitAngle[2] = { 0.0f, 0.0f };
	static float rotationAngle[2] = { 0.0f, 0.0f };

	// update rotation angles
	orbitAngle[0] += gOrbitSpeed[0] * gFrameTime;
	orbitAngle[1] += gOrbitSpeed[1] * gFrameTime;

	rotationAngle[0] += gRotationSpeed[0] * gFrameTime;
	rotationAngle[1] += gRotationSpeed[1] * gFrameTime;

	// the second object orbiting the largest one at the middle
	gModelMatrix[1] = glm::rotate(orbitAngle[0], glm::vec3(0.0f, 1.0f, 0.0f))
		* glm::translate(glm::vec3(3.0f, 0.0f, 0.0f)) 
		* glm::rotate(rotationAngle[0], glm::vec3(0.0f, 1.0f, 0.0f))
		* glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));

	glm::vec3 objectTwoPosition = glm::vec3(gModelMatrix[1][3]);

	// the third object orbiting the second object
	gModelMatrix[2] = glm::translate(objectTwoPosition)
		* glm::rotate(orbitAngle[1], glm::vec3(0.0f, 1.0f, 0.0f))
		* glm::translate(glm::vec3(1.5f, 0.0f, 0.0f)) 
		* glm::rotate(rotationAngle[1], glm::vec3(0.0f, 1.0f, 0.0f))
		* glm::scale(glm::vec3(0.25f, 0.25f, 0.25f));

	//gModelMatrixOrbitLine[0] = glm::translate(glm::vec3(-2.5f, 0.0f, 0.0f));
	gModelMatrixOrbitLine[1] = gModelMatrix[1];
}

// function to render the scene
static void render_scene()
{
	// clear colour buffer and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use the simple shader first to render the orbit lines
	gShaderOrbitLine.use();

	glViewport(130, 0, gWindowWidth, gWindowHeight); // set viewport
	glBindVertexArray(gVAO); // make VAO active

	glm::mat4 MVPOrbit;

	for (int i = 0; i < NUM_OF_ORBIT; i++) {
		MVPOrbit = gCamera.getProjMatrix() * gCamera.getViewMatrix() * gModelMatrixOrbitLine[i];
		gShaderOrbitLine.setUniform("uModelViewProjectionMatrix", MVPOrbit);

		gShader.setUniform("uModelMatrix", gModelMatrixOrbitLine[i]);
		glDrawArrays(GL_LINE_LOOP, 0, totalSlices);	// render the vertices based on primitive type
	}

	glFlush(); // flush graphics pipeline

	// use shaders associated with the shader program
	gShader.use();	

	// set light properties
	gShader.setUniform("uLight.dir", gLight.dir);
	gShader.setUniform("uLight.La", gLight.La);
	gShader.setUniform("uLight.Ld", gLight.Ld);
	gShader.setUniform("uLight.Ls", gLight.Ls);

	// set material properties based on the selected material - multiple materials
	for (int i = 0; i < NUM_OF_MODEL; i++) {
		std::string material;
		if (gMaterialSelected[i] == MaterialType::PEARL)
			material = "Pearl";
		else if (gMaterialSelected[i] == MaterialType::JADE)
			material = "Jade";
		else if (gMaterialSelected[i] == MaterialType::SOMEOTHERMATERIAL)
			material = "SomeOtherMaterial";
		gShader.setUniform("uMaterial.Ka", gMaterials[material].Ka);
		gShader.setUniform("uMaterial.Kd", gMaterials[material].Kd);
		gShader.setUniform("uMaterial.Ks", gMaterials[material].Ks);
		gShader.setUniform("uMaterial.shininess", gMaterials[material].shininess);

		// calculate matrices
		glm::mat4 MVP = gCamera.getProjMatrix() * gCamera.getViewMatrix() * gModelMatrix[i];
		glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(gModelMatrix[i])));

		// set uniform variables
		gShader.setUniform("uModelViewProjectionMatrix", MVP);
		gShader.setUniform("uModelMatrix", gModelMatrix[i]);
		gShader.setUniform("uNormalMatrix", normalMatrix);

		// render model
		gModel[i].drawModel();
	}

	// set viewing position
	gShader.setUniform("uViewpoint", glm::vec3(0.0f, 2.0f, 4.0f));

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
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		// default view
		gCamera.setViewMatrix(glm::vec3(0.0f, 3.0f, 9.0f),
			glm::vec3(0.0f, 0.0f, 0.0f));
		return;
	}
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
	{
		// front view
		gCamera.setViewMatrix(glm::vec3(0.0f, 0.0f, 9.0f),
			glm::vec3(0.0f, 0.0f, 0.0f));
		return;
	}
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
	{
		// top-down view
		gCamera.setViewMatrix(glm::vec3(0.0f, 12.0f, 0.01f),
			glm::vec3(0.0f, 0.0f, 0.0f));
		return;
	}
}

// mouse movement callback function
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	// pass cursor position to tweak bar
	TwEventMousePosGLFW(static_cast<int>(xpos), static_cast<int>(ypos));
}

// mouse button callback function
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// pass mouse button status to tweak bar
	TwEventMouseButtonGLFW(button, action);
}

// error callback function
static void error_callback(int error, const char* description)
{
	std::cerr << description << std::endl;	
}

// create and populate tweak bar elements
TwBar* create_UI(const std::string name)
{
	// create a tweak bar
	TwBar* twBar = TwNewBar(name.c_str());

	// give tweak bar the size of graphics window
	TwWindowSize(gWindowWidth, gWindowHeight);
	TwDefine(" TW_HELP visible=false ");	// disable help menu
	TwDefine(" GLOBAL fontsize=3 ");		// set large font size

	TwDefine(" Main label='User Interface' refresh=0.02 text=light size='250 450' ");

	// create frame stat entries
	TwAddVarRO(twBar, "Frame Rate", TW_TYPE_FLOAT, &gFrameRate, " group='Frame Stats' precision=2 ");
	TwAddVarRO(twBar, "Frame Time", TW_TYPE_FLOAT, &gFrameTime, " group='Frame Stats' ");

	// scene controls
	TwAddVarRW(twBar, "Wireframe", TW_TYPE_BOOLCPP, &gWireframe, " group='Controls' ");

	// object 2 control
	TwEnumVal modelValueObject2[] = {
		{static_cast<int>(ModelType::SPHERE), "Sphere"},
		{static_cast<int>(ModelType::SUZANNE), "Suzanne"},
		{static_cast<int>(ModelType::TORUS), "Torus"},
		{static_cast<int>(ModelType::CUBE), "Cube"},
		{static_cast<int>(ModelType::CONE), "Cone"},
		{static_cast<int>(ModelType::CYLINDER), "Cylinder"},
	};
	TwType modelOptionsObject2 = TwDefineEnum("modelType", modelValueObject2, 6); // last argument is the number of options
	TwAddVarRW(twBar, "Model 2", modelOptionsObject2, &gModelSelected[1], " group='Object 2' ");

	TwEnumVal materialValueObject2[] = {
		{static_cast<int>(MaterialType::PEARL), "Pearl"},
		{static_cast<int>(MaterialType::JADE), "Jade"},
		{static_cast<int>(MaterialType::SOMEOTHERMATERIAL), "SomeOtherMaterial"},
	};
	TwType materialOptionsObject2 = TwDefineEnum("materialType", materialValueObject2, 3); 
	TwAddVarRW(twBar, "Material 2", materialOptionsObject2, &gMaterialSelected[1], " group='Object 2' ");
	TwAddVarRW(twBar, "Orbit Speed 2", TW_TYPE_FLOAT, &gOrbitSpeed[0], " group='Object 2' min=-2.0 max=2.0 step=0.01");
	TwAddVarRW(twBar, "Rotation Speed 2", TW_TYPE_FLOAT, &gRotationSpeed[0], " group='Object 2' min=-2.0 max=2.0 step=0.01");

	// object 3 control
	TwEnumVal modelValueObject3[] = {
		{static_cast<int>(ModelType::SPHERE), "Sphere"},
		{static_cast<int>(ModelType::SUZANNE), "Suzanne"},
		{static_cast<int>(ModelType::TORUS), "Torus"},
		{static_cast<int>(ModelType::CUBE), "Cube"},
		{static_cast<int>(ModelType::CONE), "Cone"},
		{static_cast<int>(ModelType::CYLINDER), "Cylinder"},
	};
	TwType modelOptionsObject3 = TwDefineEnum("modelType", modelValueObject3, 6); 
	TwAddVarRW(twBar, "Model 3", modelOptionsObject3, &gModelSelected[2], " group='Object 3' ");
	
	TwEnumVal materialValueObject3[] = {
		{static_cast<int>(MaterialType::PEARL), "Pearl"},
		{static_cast<int>(MaterialType::JADE), "Jade"},
		{static_cast<int>(MaterialType::SOMEOTHERMATERIAL), "SomeOtherMaterial"},
	};
	TwType materialOptionsObject3 = TwDefineEnum("materialType", materialValueObject3, 3);
	TwAddVarRW(twBar, "Material 3", materialOptionsObject3, &gMaterialSelected[2], " group='Object 3' ");
	TwAddVarRW(twBar, "Orbit Speed 3", TW_TYPE_FLOAT, &gOrbitSpeed[1], " group='Object 3' min=-2.0 max=2.0 step=0.01");
	return twBar;
}

int main(void)
{
	GLFWwindow* window = nullptr;	

	glfwSetErrorCallback(error_callback);	

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
	window = glfwCreateWindow(gWindowWidth, gWindowHeight, "Assignment 3 - 8331030", nullptr, nullptr);

	// check if window created successfully
	if (window == nullptr)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);	
	glfwSwapInterval(1);			

	// initialise GLEW
	if (glewInit() != GLEW_OK)
	{
		// if failed to initialise GLEW
		std::cerr << "GLEW initialisation failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	// set GLFW callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// initialise scene and render settings
	init(window);

	// initialise AntTweakBar
	TwInit(TW_OPENGL_CORE, nullptr);
	TwBar* tweakBar = create_UI("Main");		

	// timing data
	double lastUpdateTime = glfwGetTime();	
	double elapsedTime = lastUpdateTime;	
	int frameCount = 0;						

	// the rendering loop
	while (!glfwWindowShouldClose(window))
	{
		update_scene(window);	// update the scene

		// if wireframe set polygon render mode to wireframe
		if (gWireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		// if model selection is changed then change the model
		for (int i = 0; i < NUM_OF_MODEL; i++)
			if (gModelSelected[i] != gPreviousModel[i]) {
				load_model(i, model_type_to_string(gModelSelected[i]));
				gPreviousModel[i] = gModelSelected[i];
				
				// refresh all the models on the lower hierarchy 
				// to avoid breaking their model rendering
				if (i + 1 != NUM_OF_MODEL) {
					// i + 1 below to skip reloading the same model on the same index
					for (int j = i + 1; j < NUM_OF_MODEL; j++) {
						load_model(j, model_type_to_string(gModelSelected[j]));
					}
				}
			}

		render_scene(); 

		// set polygon render mode to fill
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		TwDraw(); 

		glfwSwapBuffers(window); 
		glfwPollEvents(); 

		frameCount++;

		// time since last update
		elapsedTime = glfwGetTime() - lastUpdateTime;	

		if (elapsedTime > 1.0)
		{
			gFrameTime = elapsedTime / frameCount;	
			gFrameRate = 1 / gFrameTime;			
			lastUpdateTime = glfwGetTime();		
			frameCount = 0;							
		}
	}

	// uninitialise tweak bar
	TwDeleteBar(tweakBar);
	TwTerminate();

	// close the window and terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}