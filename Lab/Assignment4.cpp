#include "utilities.h"
#include "Camera.h"
#include "SimpleModel.h"
#include "stb_image.h"
#include "Texture.h"

// global variables
// settings
unsigned int gWindowWidth = 1024;
unsigned int gWindowHeight = 768;
const float gCamMoveSensitivity = 1.0f;
const float gCamRotateSensitivity = 0.1f;

// frame stats
float gFrameRate = 60.0f;
float gFrameTime = 1 / gFrameRate;

// multiple shader support 
std::map<std::string, ShaderProgram> gShader;

// multiple VAOs and VBOs
GLuint gVBO[3];
GLuint gVAO[3];

//Camera gCamera;					// camera object
std::map<int, Camera> gCamera;					// camera object
std::map<std::string, glm::mat4> gModelMatrix;	// object matrix

Light gLight;					// light properties
std::map<std::string, Material>  gMaterial;		// material properties

// textures
std::map<std::string, Texture> gTexture;

//SimpleModel gModel;				// object model
std::map<int, SimpleModel> gModel; // holder for objects' selected models

glm::mat4 viewMatrix;

// controls
bool gWireframe = false;	// wireframe control
bool gMultiView = false;	// wireframe control
float gAlpha = 0.5f;		// reflective amount
float gAlphaObject = 1.0f;
float gRotationSpeed = 1.0f;

// function initialise scene and render settings
static void init(GLFWwindow* window)
{
	// set the color the color buffer should be cleared to
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	glEnable(GL_DEPTH_TEST);	// enable depth buffer test

	// compile and link shaders
	gShader["Reflection"].compileAndLink("reflection.vert", "reflection.frag");
	gShader["NormalMap"].compileAndLink("normalMap.vert", "normalMap.frag");
	gShader["LightingTexture"].compileAndLink("lightingAndTexture.vert", "lightingAndTexture.frag");
	gShader["LightingCubemap"].compileAndLink("lightingAndCubemap.vert", "lightingAndCubemap.frag");
	gShader["Divider"].compileAndLink("divider.vert", "divider.frag");


	// ======CAMERAS======
	
	/*
	There are four cameras used to render the scenes. The first camera (gCamera[0]) is the one that 
	the user can control. The second camera (gCamera[1]) is the one on the bottom left. The third camera
	is the one on the upper right. And the fourth camera is the one used to draw the divider lines (to
	distinctly clarify the boundary for each viewport. The fourth camera uses ortographic projection
	since it will be weird to view the lines in perspective mode. 
	*/
	
	// initialise view matrix
	gCamera[0].setViewMatrix(glm::vec3(0.0f, 6.0f, 3.0f),
		glm::vec3(0.0f, 0.0f, 0.0f));

	// initialise projection matrix 
	gCamera[0].setProjMatrix(glm::perspective(glm::radians(45.0f),
		static_cast<float>(gWindowWidth) / gWindowHeight, 0.1f, 20.0f));

	/*
	Except camera 4, all cameras will use the same projection matrix, hence why the cameras are equalled to the 
	first camera to simplify the process of setting the projection matrix. The only ones different for the cameras
	are the position and the lookAt vectors, hence why the view matrix for each camera are set below. For camera 4, 
	the projection matrix and the view matrix will be different from the rest of the cameras. 
	*/
	gCamera[1] = gCamera[2] = gCamera[0];

	gCamera[1].setViewMatrix(glm::vec3(0.0f, 0.85f, 2.5f),
		glm::vec3(0.0f, 0.85f, 0.0f));

	gCamera[2].setViewMatrix(glm::vec3(0.0f, 10.5f, 0.01f),
		glm::vec3(0.0f, 0.0f, 0.0f));

	gCamera[3].setViewMatrix(glm::vec3(0.0f, 0.0f, 3.0f), 
		glm::vec3(0.0f, 0.0f, 0.0f));

	gCamera[3].setProjMatrix(glm::ortho(0.0f, static_cast<float>(gWindowWidth), 
		0.0f, static_cast<float>(gWindowHeight), 0.1f, 10.0f));

	// initialise point light properties
	gLight.pos = glm::vec3(0.0f, 2.0f, 1.0f);
	gLight.La = glm::vec3(0.3f);
	gLight.Ld = glm::vec3(1.0f);
	gLight.Ls = glm::vec3(1.0f);
	gLight.att = glm::vec3(1.0f, 0.0f, 0.0f);

	// initialise material properties
	gMaterial["Floor"].Ka = glm::vec3(0.2f);
	gMaterial["Floor"].Kd = glm::vec3(1.0f, 1.0f, 1.0f);
	gMaterial["Floor"].Ks = glm::vec3(0.2f, 0.7f, 1.0f);
	gMaterial["Floor"].shininess = 40.0f;

	gMaterial["Cube"].Ka = glm::vec3(0.25f, 0.21f, 0.21f);
	gMaterial["Cube"].Kd = glm::vec3(1.0f, 0.83f, 0.83f);
	gMaterial["Cube"].Ks = glm::vec3(0.3f, 0.3f, 0.3f);
	gMaterial["Cube"].shininess = 11.3f;

	gMaterial["Torus"].Ka = glm::vec3(0.2f);
	gMaterial["Torus"].Kd = glm::vec3(0.7f, 0.7f, 0.7f);
	gMaterial["Torus"].Ks = glm::vec3(0.2f, 0.7f, 1.0f);
	gMaterial["Torus"].shininess = 40.0f;

	// initialise material properties
	gMaterial["Wall"].Ka = glm::vec3(0.2f);
	gMaterial["Wall"].Kd = glm::vec3(0.596f, 0.765f, 1.0f);
	gMaterial["Wall"].Ks = glm::vec3(0.2f, 0.7f, 1.0f);
	gMaterial["Wall"].shininess = 40.0f;

	// initialise model matrices
	gModelMatrix["Floor"] = glm::mat4(1.0f);
	gModelMatrix["Cube"] = glm::translate(glm::vec3(-0.5f, 0.5f, 0.0f)) * glm::scale(glm::vec3(0.3f, 0.3f, 0.3f));
	gModelMatrix["Torus"] = glm::translate(glm::vec3(0.5f, 1.0f, 0.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)) 
		* glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	gModelMatrix["Wall"] = glm::mat4(1.0f);
	gModelMatrix["Divider"] = glm::mat4(1.0f);

	// load textures
	gTexture["Stone"].generate("./images/Fieldstone.bmp");
	gTexture["StoneNormalMap"].generate("./images/FieldstoneBumpDOT3.bmp");
	gTexture["Smile"].generate("./images/smile.bmp");
	gTexture["Checkerboard"].generate("./images/check.bmp");
	
	// load cube environment map texture (for torus)
	gTexture["CubeEnvironment"].generate(
		"./images/cm_front.bmp", "./images/cm_back.bmp",
		"./images/cm_left.bmp", "./images/cm_right.bmp",
		"./images/cm_top.bmp", "./images/cm_bottom.bmp");

	// load model
	gModel[0].loadModel("./models/cube.obj", true);

	// load torus model. the environment map doesn't require the model 
	// loader to flag the object as texture enabled
	gModel[1].loadModel("./models/torus.obj");

	// vertex positions and normals
	std::vector<GLfloat> floorVertices =
	{
		-3.0f, 0.0f, 3.0f,	// vertex 0: position
		0.0f, 1.0f, 0.0f,	// vertex 0: normal
		0.0f, 0.0f,			// vertex 0: texture coordinate

		3.0f, 0.0f, 3.0f,	// vertex 1: position
		0.0f, 1.0f, 0.0f,	// vertex 1: normal
		4.0f, 0.0f,			// vertex 0: texture coordinate

		-3.0f, 0.0f, -3.0f,	// vertex 2: position
		0.0f, 1.0f, 0.0f,	// vertex 2: normal
		0.0f, 4.0f,			// vertex 0: texture coordinate

		3.0f, 0.0f, -3.0f,	// vertex 3: position
		0.0f, 1.0f, 0.0f,	// vertex 3: normal
		4.0f, 4.0f,			// vertex 0: texture coordinate
	};
	std::vector<GLfloat> wallVertices = {
		-3.0f, 0.0f, -3.0f,	// vertex 0: position
		0.0f, 0.0f, 1.0f,	// vertex 0: normal
		1.0f, 0.0f, 0.0f,	// vertex 0: tangent
		0.0f, 0.0f,			// vertex 0: texture coordinate
		
		3.0f, 0.0f, -3.0f,	// vertex 1: position
		0.0f, 0.0f, 1.0f,	// vertex 1: normal
		1.0f, 0.0f, 0.0f,	// vertex 1: tangent
		4.0f, 0.0f,			// vertex 1: texture coordinate

		-3.0f, 3.0f, -3.0f,	// vertex 2: position
		0.0f, 0.0f, 1.0f,	// vertex 2: normal
		1.0f, 0.0f, 0.0f,	// vertex 2: tangent
		0.0f, 2.0f,			// vertex 2: texture coordinate

		3.0f, 3.0f, -3.0f,	// vertex 3: position
		0.0f, 0.0f, 1.0f,	// vertex 3: normal
		1.0f, 0.0f, 0.0f,	// vertex 3: tangent
		4.0f, 2.0f,			// vertex 3: texture coordinate

	};
	std::vector<GLfloat> dividerVertices = {
		0.0f, 384.0f, 0.0f,		// line 1 vertex 0: position
		1.0f, 1.0f, 1.0f,		// line 1 vertex 0: colour
		1024.0f, 384.0f, 0.0f,	// line 1 vertex 1: position
		1.0f, 1.0f, 1.0f,		// line 1 vertex 1: colour

		512.0f, 0.0f, 0.0f,		// line 2 vertex 0: position
		1.0f, 1.0f, 1.0f,		// line 2 vertex 0: colour
		512.0f, 1024.0f, 0.0f,	// line 2 vertex 1: position
		1.0f, 1.0f, 1.0f,		// line 2 vertex 1: colour
	};

	// =======FLOOR=========
	
	// create VBO
	glGenBuffers(1, &gVBO[0]);					// generate unused VBO identifier
	glBindBuffer(GL_ARRAY_BUFFER, gVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * floorVertices.size(), &floorVertices[0], GL_STATIC_DRAW);

	// create VAO, specify VBO data and format of the data
	glGenVertexArrays(1, &gVAO[0]);			// generate unused VAO identifier
	glBindVertexArray(gVAO[0]);				// create VAO
	glBindBuffer(GL_ARRAY_BUFFER, gVBO[0]);	// bind the VBO

	// use VertexNormTex since the floor is using texture
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNormTex),
		reinterpret_cast<void*>(offsetof(VertexNormTex, position)));		// specify format of position data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNormTex),
		reinterpret_cast<void*>(offsetof(VertexNormTex, normal)));		// specify format of colour data
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexNormTex),
		reinterpret_cast<void*>(offsetof(VertexNormTex, texCoord)));		// specify format of colour data

	glEnableVertexAttribArray(0);	// enable vertex attributes
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	// =======WALL=========

	// create VBO
	glGenBuffers(1, &gVBO[1]);					// generate unused VBO identifier
	glBindBuffer(GL_ARRAY_BUFFER, gVBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* wallVertices.size(), &wallVertices[0], GL_STATIC_DRAW);

	// create VAO, specify VBO data and format of the data
	glGenVertexArrays(1, &gVAO[1]);			// generate unused VAO identifier
	glBindVertexArray(gVAO[1]);				// create VAO
	glBindBuffer(GL_ARRAY_BUFFER, gVBO[1]);	// bind the VBO

	// use VertexNormTanTex because the walls use normal maps. tangent, bitangent, and the one i forgot are used 
	// for the normal map to create illusion of depth. 
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNormTanTex),
		reinterpret_cast<void*>(offsetof(VertexNormTanTex, position)));		// specify format of position data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNormTanTex),
		reinterpret_cast<void*>(offsetof(VertexNormTanTex, normal)));		// specify format of colour data
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNormTanTex),
		reinterpret_cast<void*>(offsetof(VertexNormTanTex, tangent)));		// specify format of tangent data
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexNormTanTex),
		reinterpret_cast<void*>(offsetof(VertexNormTanTex, texCoord)));		// specify format of texture coordinate data

	glEnableVertexAttribArray(0);	// enable vertex attributes
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	// =======DIVIDER=========
	
	// create VBO
	glGenBuffers(1, &gVBO[2]);					// generate unused VBO identifier
	glBindBuffer(GL_ARRAY_BUFFER, gVBO[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* dividerVertices.size(), &dividerVertices[0], GL_STATIC_DRAW);

	// create VAO, specify VBO data and format of the data
	glGenVertexArrays(1, &gVAO[2]);			// generate unused VAO identifier
	glBindVertexArray(gVAO[2]);				// create VAO
	glBindBuffer(GL_ARRAY_BUFFER, gVBO[2]);	// bind the VBO

	// use simple VertexColor since it only draws line 
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor),
		reinterpret_cast<void*>(offsetof(VertexColor, position)));		// specify format of position data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor),
		reinterpret_cast<void*>(offsetof(VertexColor, color)));		// specify format of colour data

	glEnableVertexAttribArray(0);	// enable vertex attributes
	glEnableVertexAttribArray(1);
}

// function used to update the scene
static void update_scene(GLFWwindow* window)
{
	// stores camera forward/back, up/down and left/right movements
	float moveForward = 0.0f;
	float moveRight = 0.0f;
	float rotationAngle = 0.0f; // for rotating torus

	rotationAngle += gRotationSpeed * gFrameTime;

	// update movement variables based on keyboard input
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		moveForward += gCamMoveSensitivity * gFrameTime;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		moveForward -= gCamMoveSensitivity * gFrameTime;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		moveRight -= gCamMoveSensitivity * gFrameTime;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		moveRight += gCamMoveSensitivity * gFrameTime;

	// update camera position and direction
	gCamera[0].update(moveForward, moveRight);

	// rotate torus indefinitely 
	gModelMatrix["Torus"] *= glm::rotate(rotationAngle, glm::vec3(0.0f, 0.0f, 1.0f));
}

void draw_floor(float alpha, Camera gCamera)
{
	// use the shaders associated with the shader program
	gShader["Reflection"].use();

	// set light properties
	gShader["Reflection"].setUniform("uLight.pos", gLight.pos);
	gShader["Reflection"].setUniform("uLight.La", gLight.La);
	gShader["Reflection"].setUniform("uLight.Ld", gLight.Ld);
	gShader["Reflection"].setUniform("uLight.Ls", gLight.Ls);
	gShader["Reflection"].setUniform("uLight.att", gLight.att);

	// set viewing position
	gShader["Reflection"].setUniform("uViewpoint", gCamera.getPosition());

	// set material properties
	gShader["Reflection"].setUniform("uMaterial.Ka", gMaterial["Floor"].Ka);
	gShader["Reflection"].setUniform("uMaterial.Kd", gMaterial["Floor"].Kd);
	gShader["Reflection"].setUniform("uMaterial.Ks", gMaterial["Floor"].Ks);
	gShader["Reflection"].setUniform("uMaterial.shininess", gMaterial["Floor"].shininess);

	// calculate matrices
	glm::mat4 MVP = gCamera.getProjMatrix() * gCamera.getViewMatrix() * gModelMatrix["Floor"];
	glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(gModelMatrix["Floor"])));

	// set uniform variables
	gShader["Reflection"].setUniform("uModelViewProjectionMatrix", MVP);
	gShader["Reflection"].setUniform("uModelMatrix", gModelMatrix["Floor"]);
	gShader["Reflection"].setUniform("uNormalMatrix", normalMatrix);

	// set blending amount
	gShader["Reflection"].setUniform("uAlpha", alpha);
	gShader["Reflection"].setUniform("uTextureSampler", 3);

	// add texture to the floor 
	glActiveTexture(GL_TEXTURE3);
	gTexture["Checkerboard"].bind();

	glBindVertexArray(gVAO[0]);				// make vao 0 active, the one for the floor
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);	// render the vertices
}

void draw_objects(bool reflection, Camera gCamera)
{
	glm::mat4 MVP = glm::mat4(1.0f);
	glm::mat3 normalMatrix = glm::mat3(1.0f);
	glm::mat4 reflectMatrix = glm::mat4(1.0f);
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	glm::vec3 lightPosition = gLight.pos;

	if (reflection)
	{
		// create reflection matrix about the horizontal plane
		reflectMatrix = glm::scale(glm::vec3(1.0f, -1.0f, 1.0f));
		// reposition the point light when rendering the reflection
		lightPosition = glm::vec3(reflectMatrix * glm::vec4(lightPosition, 1.0f));
	}

	// =======CUBE=========
	
	// use lighting texture shader for the cube (texure and lighting) 
	gShader["LightingTexture"].use();

	// set light properties
	gShader["LightingTexture"].setUniform("uLight.pos", lightPosition);
	gShader["LightingTexture"].setUniform("uLight.La", gLight.La);
	gShader["LightingTexture"].setUniform("uLight.Ld", gLight.Ld);
	gShader["LightingTexture"].setUniform("uLight.Ls", gLight.Ls);
	gShader["LightingTexture"].setUniform("uLight.att", gLight.att);

	// set viewing position
	gShader["LightingTexture"].setUniform("uViewpoint", gCamera.getPosition());

	// set material properties
	gShader["LightingTexture"].setUniform("uMaterial.Ka", gMaterial["Cube"].Ka);
	gShader["LightingTexture"].setUniform("uMaterial.Kd", gMaterial["Cube"].Kd);
	gShader["LightingTexture"].setUniform("uMaterial.Ks", gMaterial["Cube"].Ks);
	gShader["LightingTexture"].setUniform("uMaterial.shininess", gMaterial["Cube"].shininess);

	// calculate matrices
	modelMatrix = reflectMatrix * gModelMatrix["Cube"];
	MVP = gCamera.getProjMatrix() * gCamera.getViewMatrix() * modelMatrix;
	normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

	// set uniform variables
	gShader["LightingTexture"].setUniform("uModelViewProjectionMatrix", MVP);
	gShader["LightingTexture"].setUniform("uModelMatrix", modelMatrix);
	gShader["LightingTexture"].setUniform("uNormalMatrix", normalMatrix);

	gShader["LightingTexture"].setUniform("uTextureSampler", 2);

	glActiveTexture(GL_TEXTURE2);
	gTexture["Smile"].bind();

	// draw model
	gModel[0].drawModel();

	// =======TORUS=========

	// use the lighting cube map for the torus (environment map)
	gShader["LightingCubemap"].use();

	// set light properties
	gShader["LightingCubemap"].setUniform("uLight.pos", lightPosition);
	gShader["LightingCubemap"].setUniform("uLight.La", gLight.La);
	gShader["LightingCubemap"].setUniform("uLight.Ld", gLight.Ld);
	gShader["LightingCubemap"].setUniform("uLight.Ls", gLight.Ls);
	gShader["LightingCubemap"].setUniform("uLight.att", gLight.att);

	// set viewing position
	gShader["LightingCubemap"].setUniform("uViewpoint", gCamera.getPosition());

	// set material properties
	gShader["LightingCubemap"].setUniform("uMaterial.Ka", gMaterial["Torus"].Ka);
	gShader["LightingCubemap"].setUniform("uMaterial.Kd", gMaterial["Torus"].Kd);
	gShader["LightingCubemap"].setUniform("uMaterial.Ks", gMaterial["Torus"].Ks);
	gShader["LightingCubemap"].setUniform("uMaterial.shininess", gMaterial["Torus"].shininess);

	// calculate matrices
	modelMatrix = reflectMatrix * gModelMatrix["Torus"];
	MVP = gCamera.getProjMatrix() * gCamera.getViewMatrix() * modelMatrix;
	normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

	// set uniform variables
	gShader["LightingCubemap"].setUniform("uModelViewProjectionMatrix", MVP);
	gShader["LightingCubemap"].setUniform("uModelMatrix", modelMatrix);
	gShader["LightingCubemap"].setUniform("uNormalMatrix", normalMatrix);

	// set cube environment map
	gShader["LightingCubemap"].setUniform("uEnvironmentMap", 4);
	gShader["LightingCubemap"].setUniform("uAlpha", gAlphaObject);

	glActiveTexture(GL_TEXTURE4);
	gTexture["CubeEnvironment"].bind();

	gModel[1].drawModel();

	// =======WALL=========

	// use the normal map shader (texture and normal map)
	gShader["NormalMap"].use();

	// set light properties
	gShader["NormalMap"].setUniform("uLight.pos", lightPosition);
	gShader["NormalMap"].setUniform("uLight.La", gLight.La);
	gShader["NormalMap"].setUniform("uLight.Ld", gLight.Ld);
	gShader["NormalMap"].setUniform("uLight.Ls", gLight.Ls);
	gShader["NormalMap"].setUniform("uLight.att", gLight.att);

	// set material properties
	gShader["NormalMap"].setUniform("uMaterial.Ka", gMaterial["Wall"].Ka);
	gShader["NormalMap"].setUniform("uMaterial.Kd", gMaterial["Wall"].Kd);
	gShader["NormalMap"].setUniform("uMaterial.Ks", gMaterial["Wall"].Ks);
	gShader["NormalMap"].setUniform("uMaterial.shininess", gMaterial["Wall"].shininess);

	// set viewing position
	gShader["NormalMap"].setUniform("uViewpoint", gCamera.getPosition());

	glBindVertexArray(gVAO[1]);				// make VAO active

	// draw multiple walls at once that surrounds the floor (size of the wall is specified in the vertices array) 
	for (int i = 0; i < 4; i++) {
		modelMatrix = reflectMatrix * gModelMatrix["Wall"];
		MVP = gCamera.getProjMatrix() * gCamera.getViewMatrix() * modelMatrix;
		normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
		
		gShader["NormalMap"].setUniform("uModelViewProjectionMatrix", MVP);
		gShader["NormalMap"].setUniform("uModelMatrix", modelMatrix);
		gShader["NormalMap"].setUniform("uNormalMatrix", normalMatrix);

		// set texture and normal map
		gShader["NormalMap"].setUniform("uTextureSampler", 0);
		gShader["NormalMap"].setUniform("uNormalSampler", 1);

		glActiveTexture(GL_TEXTURE0);
		gTexture["Stone"].bind();

		glActiveTexture(GL_TEXTURE1);
		gTexture["StoneNormalMap"].bind();

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		gModelMatrix["Wall"] *= glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}
}

static void draw_scene(Camera gCamera) {
	/************************************************************************************
	 * Disable colour buffer and depth buffer, and draw reflective surface into stencil buffer
	 ************************************************************************************/
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);  // disable any modification to all colour components
	glDepthMask(GL_FALSE);                                // disable any modification to depth value
	glEnable(GL_STENCIL_TEST);                            // enable stencil testing

	// setup the stencil buffer with a reference value
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	// draw the reflective surface into the stencil buffer
	draw_floor(1.0f, gCamera);

	/************************************************************************************
	 * Enable colour buffer and depth buffer, draw reflected geometry where stencil test passes
	 ************************************************************************************/
	 // only render where stencil buffer equals to 1
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);   // allow all colour components to be modified 
	glDepthMask(GL_TRUE);                              // allow depth value to be modified

	// draw the reflected objects
	draw_objects(true, gCamera);

	glDisable(GL_STENCIL_TEST);		// disable stencil testing

	/************************************************************************************
	 * Draw the scene
	 ************************************************************************************/
	 // draw reflective surface by blending with reflection
	glEnable(GL_BLEND);		//enable blending            
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

	// blend reflective surface with reflection
	draw_floor(gAlpha, gCamera);

	glDisable(GL_BLEND);	//disable blending

	// draw the normal scene
	draw_objects(false, gCamera);
}

// function to render the scene
static void render_scene()
{
	/************************************************************************************
	 * Clear colour buffer, depth buffer and stencil buffer
	 ************************************************************************************/
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// if multiview is enabled, draw 3 viewports to the scene, else just draw the main one
	if (gMultiView) {
		glViewport(0, 0, 512, 384);
		draw_scene(gCamera[1]);

		glViewport(512, 0, 512, 384);
		draw_scene(gCamera[0]);

		glViewport(512, 384, 512, 384);
		draw_scene(gCamera[2]);

		gShader["Divider"].use();
		glm::mat4 MVPDivider = gCamera[3].getProjMatrix()
			* gCamera[3].getViewMatrix()
			* gModelMatrix["Divider"];
		gShader["Divider"].setUniform("uModelViewProjectionMatrix", MVPDivider);

		glBindVertexArray(gVAO[2]);
		glViewport(0, 0, gWindowWidth, gWindowHeight);
		glDrawArrays(GL_LINES, 0, 4);	// display the lines
	}
	else {
		draw_scene(gCamera[0]);
	}

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
}

// mouse movement callback function
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	// pass cursor position to tweak bar
	TwEventMousePosGLFW(static_cast<int>(xpos), static_cast<int>(ypos));

	// previous cursor coordinates
	static glm::vec2 previousPos = glm::vec2(xpos, ypos);
	static int counter = 0;

	// allow camera rotation when right mouse button held
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		// stablise cursor coordinates for 5 updates
		if (counter < 5)
		{
			// set previous cursor coordinates
			previousPos = glm::vec2(xpos, ypos);
			counter++;
		}

		// change based on previous cursor coordinates
		float deltaYaw = (previousPos.x - xpos) * gCamRotateSensitivity * gFrameTime;
		float deltaPitch = (previousPos.y - ypos) * gCamRotateSensitivity * gFrameTime;

		// update camera's yaw and pitch
		gCamera[0].updateRotation(deltaYaw, deltaPitch);

		// set previous cursor coordinates
		previousPos = glm::vec2(xpos, ypos);
	}
	else
	{
		counter = 0;
	}
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
	std::cerr << description << std::endl;	// output error description
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

	TwDefine(" Main label='User Interface' refresh=0.02 text=light size='250 350' position='10 10' ");

	// create frame stat entries
	TwAddVarRO(twBar, "Frame Rate", TW_TYPE_FLOAT, &gFrameRate, " group='Frame Stats' precision=2 ");
	TwAddVarRO(twBar, "Frame Time", TW_TYPE_FLOAT, &gFrameTime, " group='Frame Stats' ");

	// scene controls
	TwAddVarRW(twBar, "Wireframe", TW_TYPE_BOOLCPP, &gWireframe, " group='Controls' ");
	TwAddVarRW(twBar, "MultiView", TW_TYPE_BOOLCPP, &gMultiView, " group='Controls' ");

	// light control
	TwAddVarRW(twBar, "Position X", TW_TYPE_FLOAT, &gLight.pos.x, " group='Light' min=-3 max=3 step=0.01 ");
	TwAddVarRW(twBar, "Position Y", TW_TYPE_FLOAT, &gLight.pos.y, " group='Light' min=-3 max=3 step=0.01 ");
	TwAddVarRW(twBar, "Position Z", TW_TYPE_FLOAT, &gLight.pos.z, " group='Light' min=-3 max=3 step=0.01 ");

	// transformation
	TwAddVarRW(twBar, "Rotation Speed", TW_TYPE_FLOAT, &gRotationSpeed, " group='Transformation' min=-3 max=3 step=0.01 ");

	// reflective amount
	TwAddVarRW(twBar, "Floor", TW_TYPE_FLOAT, &gAlpha, " group='Reflection' min=0.2 max=1 step=0.01 ");
	TwAddVarRW(twBar, "Object", TW_TYPE_FLOAT, &gAlphaObject, " group='Reflection' min=0.2 max=1 step=0.01 ");

	return twBar;
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
	window = glfwCreateWindow(gWindowWidth, gWindowHeight, "Assignment 4 - 8331030", nullptr, nullptr);

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
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// initialise scene and render settings
	init(window);

	// initialise AntTweakBar
	TwInit(TW_OPENGL_CORE, nullptr);
	TwBar* tweakBar = create_UI("Main");		// create and populate tweak bar elements

	// timing data
	double lastUpdateTime = glfwGetTime();	// last update time
	double elapsedTime = lastUpdateTime;	// time since last update
	int frameCount = 0;						// number of frames since last update

	// the rendering loop
	while (!glfwWindowShouldClose(window))
	{
		update_scene(window);	// update the scene

		// if wireframe set polygon render mode to wireframe
		if (gWireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		render_scene();			// render the scene

		// set polygon render mode to fill
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		TwDraw();				// draw tweak bar

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
	glDeleteBuffers(1, &gVBO[0]);
	glDeleteVertexArrays(1, &gVAO[0]);
	glDeleteBuffers(1, &gVBO[1]);
	glDeleteVertexArrays(1, &gVAO[1]);

	// uninitialise tweak bar
	TwDeleteBar(tweakBar);
	TwTerminate();

	// close the window and terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}