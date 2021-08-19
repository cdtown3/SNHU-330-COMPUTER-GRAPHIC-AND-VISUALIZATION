/*
	Program:     CSS-330: Final Project
	Author:      Drew Townsend
	Date:        08/13/21
	Description: This program renders a 3D scene based off of a 2D image. The scene contains a Table top, Rubik's Cube, Book,
				 Perfume bottle, and candle. I ended up utilizing the cylinder class created by Song Ho Ahn. I also implemented
				 a fragment shader that contains logic for point lights. There are 2 point lights in the scene. An all white one
				 on the right side and a green one on the left.

	Controls:
	WASD -							[Control forward and side movement of camera]
	Mouse -							[Control panning of camera]
	ScrollWheel -					[Control speed of movement]
	E -								[Increase camera's Z axis]
	Q -								[Decrease camera's Z axis]
	P -								[*Sensitive to press* Changes Projection]

*/

// including libraries
#include <iostream>
#include <cstdlib>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
// #include <glad/glad.h>

// GLM Math Headers
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// cylinder class
#include "Dependencies/cylinder/Cylinder.h"

using namespace std;

// Shader program macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "Drew Townsend - Module 6 Milestone"; // Window title

	// Window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	// Declaring unsigned ints for vertex array and buffer, as well as number of indices
	struct GLMesh
	{
		GLuint vaos[2];
		GLuint vbos[2];
		GLuint nIndices;
	};

	// defining main window
	GLFWwindow* gWindow = nullptr;
	// Triangle mesh data
	GLMesh gMesh;
	// Texture IDs
	GLuint texture0, texture1, texture2, texture3, texture4;
	// defining both shader programs
	GLuint gProgramId, gCylProgramId;

	// global cam variables
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// deltaTime ensures all users have similar movement speed
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;

	// initial mouse coords
	float lastX = 400, lastY = 300;

	// values for mouse input
	float yaw = 0.0f, pitch = 0.0f;
	bool firstMouse = true;
	float sensitivity = 0.1f;
	float scrollSpeed = 0.1f;

	// bool to change perspective to ortho
	bool perspective = true;

	// Checking to see if projection was changed on last frame
	bool lastFrameCheck = false;

	// light color
	glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);
	glm::vec3 gLightColor2(0.8f, 1.0f, 0.8f);

	glm::vec3 pointLightColors[2] = {
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(0.5f, 1.0f, 0.5f)
	};

	// Light position and scale
	float lX = 1.4f, lY = 0.04f, lZ = 3.5f; // Allows light source variables to be changed
	glm::vec3 gLightPosition(lX, lY, lZ);
	glm::vec3 gLightScale(0.3f);

	// Cylinders
	Cylinder cylinder1(1.0f, 1.1f, 2.0f, 360, 1);
	Cylinder cylinder2(1.0f, 1.0f, 2.0f, 360, 1);
}

// Initializes libraries and window/context
bool UInitialize(int, char* [], GLFWwindow** window);
// Resizes active window if user or program changes size
void UResizeWindow(GLFWwindow* window, int width, int height);
// Processes input (like a keyboard key) and does something with it
void UProcessInput(GLFWwindow* window);
// Mouse scroll callback
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
// Setting index locations, colors, etc...
void UCreateMesh(GLMesh& mesh);
// Destroys locations
void UDestroyMesh(GLMesh& mesh);
// Actually renders the pyramid and allows for transformations
void URender();
// Creates, compiles, and deleted shader programs (when error occurs)
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
// Deleting shader programs
void UDestroyShaderProgram(GLuint programId);
// Captures mouse events commented out for now
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// Loads texture for placing
bool UCreateTexture(const char* filename, GLuint& textureId);
// Deallocates memory from texture
void UDestroyTexture(GLuint textureId);
// Flipping image on Y axis
void flipImageVertically(unsigned char* image, int width, int height, int channels);


// Vertex Shader Source Code
const GLchar* objectVertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position;
	layout(location = 1) in vec3 normal;
	layout(location = 2) in vec2 textureCoordinate;

	out vec3 vertexNormal;
	out vec3 vertexFragmentPos;
	out vec2 vertexTextureCoordinate;

	// variables to be used for transforming
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main()
	{
		gl_Position = projection * view * model * vec4(position, 1.0f); // transforming vertices to clip coords

		vertexFragmentPos = vec3(model * vec4(position, 1.0f));

		vertexNormal = mat3(transpose(inverse(model))) * normal;
		vertexTextureCoordinate = textureCoordinate; // receives color data
	}
);



// Fragment shader source code
const GLchar* objectFragmentShaderSource = GLSL(440,
	in vec3 vertexNormal; // For incoming normals
	in vec3 vertexFragmentPos; // For incoming fragment position
	in vec2 vertexTextureCoordinate; // For incoming texture coordinates

	// Implementing attenuation
	struct Light {
		vec3 position;

		vec3 ambient;
		vec3 diffuse;
		vec3 specular;

		float constant;
		float linear;
		float quadratic;
	};

	out vec4 fragmentColor; // For outgoing pyramid color to the GPU

	// Uniform / Global variables for object color, light color, light position, and camera/view position
	uniform vec3 lightPos;
	uniform vec3 viewPosition;
	uniform sampler2D uTexture; // Useful when working with multiple textures
	uniform Light pointLights[2];

	layout(binding = 3) uniform sampler2D texSampler1;

	// function contains logic for pointlights, and outputs a light source containing ambient, diffuse, and specular lighting
	vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir);

	void main()
	{
		/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/
		vec3 norm = normalize(vertexNormal);
		vec3 viewDir = normalize(viewPosition - vertexFragmentPos);
		vec3 result;
		for (int i = 0; i < 2; i++)
		result += CalcPointLight(pointLights[i], norm, vertexFragmentPos, viewDir);

		fragmentColor = vec4(result, 1.0);
	}

	vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
	{
		float highlightSize = 32.0f;
		vec3 lightDir = normalize(light.position - fragPos);

		float diff = max(dot(normal, lightDir), 0.0);
		vec3 reflectDir = reflect(-lightDir, normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);

		float distance = length(light.position - fragPos);
		float attenuation = 1.0 / (light.constant + light.linear * distance +
							light.quadratic * (distance * distance));

		vec3 ambient = light.ambient * vec3(texture(uTexture, vertexTextureCoordinate).rgb);
		vec3 diffuse = light.diffuse * diff * vec3(texture(uTexture, vertexTextureCoordinate).rgb);
		vec3 specular = light.specular * spec * vec3(texture(uTexture, vertexTextureCoordinate).rgb);
		ambient *= attenuation;
		diffuse *= attenuation;
		specular *= attenuation;
		return (ambient + diffuse + specular);
	}

);

int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	UCreateMesh(gMesh);

	if (!UCreateShaderProgram(objectVertexShaderSource, objectFragmentShaderSource, gProgramId))
		return EXIT_FAILURE;
	if (!UCreateShaderProgram(objectVertexShaderSource, objectFragmentShaderSource, gCylProgramId))
		return EXIT_FAILURE;

	// Telling OpenGL which texture the sample is connected to, which is unit 0
	glUseProgram(gProgramId);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// render loop
	while (!glfwWindowShouldClose(gWindow))
	{
		UProcessInput(gWindow);		

		// bind texture on corresponding texture unit
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texture2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, texture3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, texture4);

		URender();

		glfwPollEvents();

		glfwSetCursorPosCallback(gWindow, mouse_callback);
	}

	UDestroyMesh(gMesh);

	// release textures
	UDestroyTexture(texture0);
	UDestroyTexture(texture1);
	UDestroyTexture(texture2);
	UDestroyTexture(texture3);
	UDestroyTexture(texture4);

	UDestroyShaderProgram(gProgramId);
	UDestroyShaderProgram(gCylProgramId);

	exit(EXIT_SUCCESS);
}

bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	glfwInit(); // initializing GLFW library
	// Setting OpenGL versions
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	// Core profile allows access to subset of OpenGL features without backwards-compatibility
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// If device is using MacOS, enable forward compatibility
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Creating GLFW window using previously defined variables
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	// If creation fails, alert user and terminate
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	// setting window context as current and setting framebuffer resize callback of window
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetScrollCallback(*window, UMouseScrollCallback);

	// When window has focus on PC, disable mouse cursor
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Enabling use of experimental/pre-release drivers
	glewExperimental = GL_TRUE;
	// initializing GLEW library
	GLenum GlewInitResult = glewInit();
	// Ensuring GLEW properly initialized
	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displaying GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}

// if declared key(s) are pressed during this frame, do something
void UProcessInput(GLFWwindow* window)
{
	// Checking if 'escape' key was pressed. If so, close window.
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	const float cameraSpeed = 2.5f * scrollSpeed;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraUp;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraUp;

	// press "p" to change projections
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
	{
		// Checking to see if projection was changed on last frame
		if (!lastFrameCheck)
		{
			perspective = !perspective;
			cout << "Projection Changed" << endl;
			lastFrameCheck = true;
		}
		else
		{
			lastFrameCheck = false;
		}

	}
}


void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (yoffset > 0)
	{
		scrollSpeed += 0.01;
	}
	else
	{
		if (scrollSpeed > 0.01) // limiting speed to a minimum of 0.
		{
			scrollSpeed -= 0.01;
		}
	}
	cout << scrollSpeed << endl;
}

// Set viewport if window is resized
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// URender will render the frame. This function is in the while loop within main()
void URender()
{
	// Lamp orbits around the origin
	gLightPosition.x = lX;
	gLightPosition.y = lY;
	gLightPosition.z = lZ;

	// Enabling z-depth
	glEnable(GL_DEPTH_TEST);

	// Clear frame to black, clear the z buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Activate VAO
	glBindVertexArray(gMesh.vaos[0]);

	// 1. Scales the object by 2
	glm::mat4 scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
	// 2. Place object at the origin
	glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
	// 3. Rotate object
	glm::mat4 rotation = glm::rotate(0.6f, glm::vec3(0, 0, 1));
	// Model matrix: transformations are applied right-to-left order
	glm::mat4 model = translation * scale;

	// Defining perspective projection to start, however pressing P will change perspective to ortho
	glm::mat4 projection = glm::perspective(1.0f, GLfloat(WINDOW_WIDTH / WINDOW_HEIGHT), 0.1f, 100.0f);
	if (perspective)
	{
		projection = glm::perspective(1.0f, GLfloat(WINDOW_WIDTH / WINDOW_HEIGHT), 0.1f, 100.0f);
	}
	else
	{
		projection = glm::ortho(0.0f, 5.0f, 0.0f, 5.0f, 0.1f, 100.0f);
	}

	const float radius = 10.0f;
	float camX = sin(glfwGetTime()) * radius;
	float camZ = cos(glfwGetTime()) * radius;

	// new camera view that allows movement. commented out for now
	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	glUseProgram(gProgramId);

	// Program 1
	GLint modelLoc = glGetUniformLocation(gProgramId, "model");
	GLint viewLoc = glGetUniformLocation(gProgramId, "view");
	GLint projLoc = glGetUniformLocation(gProgramId, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Reference matrix uniforms for Shader program
	GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
	GLint lightAmbientLoc = glGetUniformLocation(gProgramId, "pointLights[0].ambient");
	GLint lightDiffuseLoc = glGetUniformLocation(gProgramId, "pointLights[0].diffuse");
	GLint lightSpecularLoc = glGetUniformLocation(gProgramId, "pointLights[0].specular");
	GLint lightPositionLoc = glGetUniformLocation(gProgramId, "pointLights[0].position");
	GLfloat lightConstantLoc = glGetUniformLocation(gProgramId, "pointLights[0].constant");
	GLfloat lightLinearLoc = glGetUniformLocation(gProgramId, "pointLights[0].linear");
	GLfloat lightQuadraticLoc = glGetUniformLocation(gProgramId, "pointLights[0].quadratic");
	// light2
	GLint lightAmbientLoc2 = glGetUniformLocation(gProgramId, "pointLights[1].ambient");
	GLint lightDiffuseLoc2 = glGetUniformLocation(gProgramId, "pointLights[1].diffuse");
	GLint lightSpecularLoc2 = glGetUniformLocation(gProgramId, "pointLights[1].specular");
	GLint lightPositionLoc2 = glGetUniformLocation(gProgramId, "pointLights[1].position");
	GLfloat lightConstantLoc2 = glGetUniformLocation(gProgramId, "pointLights[1].constant");
	GLfloat lightLinearLoc2 = glGetUniformLocation(gProgramId, "pointLights[1].linear");
	GLfloat lightQuadraticLoc2 = glGetUniformLocation(gProgramId, "pointLights[1].quadratic");

	// Pass color, light, and camera data to the pyramid Shader program's corresponding uniforms
	glUniform3f(lightPositionLoc, 1.4f, 0.04f, 3.5f);
	const glm::vec3 cameraPosition = (cameraPos, cameraPos + cameraFront, cameraUp);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
	// Setting float variables for caster
	glUniform3f(lightAmbientLoc, pointLightColors[0].x * 0.1f, pointLightColors[0].y * 0.1f, pointLightColors[0].z * 0.1f);
	glUniform3f(lightDiffuseLoc, pointLightColors[0].x, pointLightColors[0].y, pointLightColors[0].z);
	glUniform3f(lightSpecularLoc, pointLightColors[0].x, pointLightColors[0].y, pointLightColors[0].z);
	glUniform1f(lightConstantLoc, 1.0f);
	glUniform1f(lightLinearLoc, 0.09f);
	glUniform1f(lightQuadraticLoc, 0.032f);
	// light 2
	glUniform3f(lightAmbientLoc2, pointLightColors[1].x * 0.1f, pointLightColors[1].y * 0.1f, pointLightColors[1].z * 0.1f);
	glUniform3f(lightDiffuseLoc2, pointLightColors[1].x, pointLightColors[1].y, pointLightColors[1].z);
	glUniform3f(lightSpecularLoc2, pointLightColors[1].x, pointLightColors[1].y, pointLightColors[1].z);
	glUniform3f(lightPositionLoc2, -6.1f, 2.0f, 4.2f);
	glUniform1f(lightConstantLoc2, 1.0f);
	glUniform1f(lightLinearLoc2, 0.09f);
	glUniform1f(lightQuadraticLoc2, 0.032f);

	glBindVertexArray(gMesh.vaos[0]);

	// Loading textures and drawing objects

	/********************
	*     Table top 	*
	********************/

	// Marble texture
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);
	// Tabletop vertices
	glDrawArrays(GL_TRIANGLES, 0, 6);

	/********************
	*        Book		*
	********************/

	// Book Cover Texture
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 1);
	// Book
	glDrawArrays(GL_TRIANGLES, 6, 36);

	/********************
	*   Rubik's Cube	*
	********************/

	// Rubik's Cube Texture
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);
	// Cube
	glDrawArrays(GL_TRIANGLES, 42, 36);

	/********************
	*  Perfume Bottle	*
	********************/
	scale = glm::scale(glm::vec3(0.7f, 0.7f, 0.7f));
	rotation = glm::rotate(0.6f, glm::vec3(0, 0, 1));
	translation = glm::translate(glm::vec3(0, 0, 0));

	model = translation * rotation * scale;

	// Pass new matrix data from model
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Dust Texture
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 3);
	// Perfume bottle
	glDrawArrays(GL_TRIANGLES, 78, 36);

	/********************
	*     Perfume Cap	*
	********************/

	scale = glm::scale(glm::vec3(0.26f, 0.26f, 0.7f));
	rotation = glm::rotate(3.79f, glm::vec3(-0.05f, 0.0f, 3.3f));
	translation = glm::translate(glm::vec3(-0.53f, -0.13f, 0));

	model = translation * rotation * scale;

	// Pass new matrix data from model
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawArrays(GL_TRIANGLES, 114, 36);

	/********************
	*     Cylinder 1	*
	********************/

	glUseProgram(gCylProgramId);
	glBindVertexArray(gMesh.vaos[1]);

	// Program 2
	modelLoc = glGetUniformLocation(gCylProgramId, "model");
	viewLoc = glGetUniformLocation(gCylProgramId, "view");
	projLoc = glGetUniformLocation(gCylProgramId, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Reference matrix uniforms for Shader program
	lightPositionLoc = glGetUniformLocation(gCylProgramId, "lightPos");
	viewPositionLoc = glGetUniformLocation(gCylProgramId, "viewPosition");
	lightConstantLoc = glGetUniformLocation(gCylProgramId, "light.constant");
	lightLinearLoc = glGetUniformLocation(gCylProgramId, "light.linear");
	lightQuadraticLoc = glGetUniformLocation(gCylProgramId, "light.quadratic");
	lightAmbientLoc = glGetUniformLocation(gCylProgramId, "light.ambient");
	lightDiffuseLoc = glGetUniformLocation(gCylProgramId, "light.diffuse");
	lightSpecularLoc = glGetUniformLocation(gCylProgramId, "light.specular");
	// light2
	lightAmbientLoc2 = glGetUniformLocation(gCylProgramId, "pointLights[1].ambient");
	lightDiffuseLoc2 = glGetUniformLocation(gCylProgramId, "pointLights[1].diffuse");
	lightSpecularLoc2 = glGetUniformLocation(gCylProgramId, "pointLights[1].specular");
	lightPositionLoc2 = glGetUniformLocation(gCylProgramId, "pointLights[1].position");
	lightConstantLoc2 = glGetUniformLocation(gCylProgramId, "pointLights[1].constant");
	lightLinearLoc2 = glGetUniformLocation(gCylProgramId, "pointLights[1].linear");
	lightQuadraticLoc2 = glGetUniformLocation(gCylProgramId, "pointLights[1].quadratic");

	// Pass color, light, and camera data to the pyramid Shader program's corresponding uniforms
	glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
	// const glm::vec3 cameraPosition = (cameraPos, cameraPos + cameraFront, cameraUp);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
	// Setting float variables for caster
	glUniform3f(lightAmbientLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(lightDiffuseLoc, 0.8f, 0.8f, 0.8f);
	glUniform3f(lightSpecularLoc, 1.0f, 1.0f, 1.0f);
	glUniform1f(lightConstantLoc, 1.0f);
	glUniform1f(lightLinearLoc, 0.09f);
	glUniform1f(lightQuadraticLoc, 0.032f);
	// light 2
	glUniform3f(lightAmbientLoc2, pointLightColors[1].x * 0.1f, pointLightColors[1].y * 0.1f, pointLightColors[1].z * 0.1f);
	glUniform3f(lightDiffuseLoc2, pointLightColors[1].x, pointLightColors[1].y, pointLightColors[1].z);
	glUniform3f(lightSpecularLoc2, pointLightColors[1].x, pointLightColors[1].y, pointLightColors[1].z);
	glUniform3f(lightPositionLoc2, -6.1f, 2.0f, 4.2f);
	glUniform1f(lightConstantLoc2, 1.0f);
	glUniform1f(lightLinearLoc2, 0.09f);
	glUniform1f(lightQuadraticLoc2, 0.032f);

	scale = glm::scale(glm::vec3(0.2f, 0.2f, 0.2f));
	rotation = glm::rotate(3.15f, glm::vec3(8.0f, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(-0.3f, -1.4f, 0.21f));

	model = translation * rotation * scale;

	// Pass new matrix data from model
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	cylinder1.draw();

	/********************
	*     Cylinder 2	*
	********************/

	scale = glm::scale(glm::vec3(0.04f, 0.04f, 0.04f));
	rotation = glm::rotate(2.0f, glm::vec3(0.6f, -0.06f, 0.45f));
	translation = glm::translate(glm::vec3(-0.44f, -0.25f, 0.0f));

	model = translation * rotation * scale;

	// Pass new matrix data from model
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	cylinder2.draw();

	glBindVertexArray(0);

	glfwSwapBuffers(gWindow);
}

// UCreateMesh contains positions and color data, and ensures data is in GPU memory
void UCreateMesh(GLMesh& mesh)
{
	 // Position, Normal, and texture data for objects
	GLfloat verts[] = {
		// Table top			// Plane Normal		// Texture Coords
		-1.0f, -1.0f,   0.0f,	0.0f, 0.0f, 1.0f,	0.0f,  0.0f, // bottom left vertex
		-1.0f,  1.0f,   0.0f,	0.0f, 0.0f, 1.0f,	0.0f,  1.0f, // top left vertex
		 1.0f,  1.0f,   0.0f,	0.0f, 0.0f, 1.0f,	1.0f,  1.0f, // top right vertex
		 1.0f,  1.0f,   0.0f,	0.0f, 0.0f, 1.0f,	1.0f,  1.0f, // top right vertex
		 1.0f, -1.0f,   0.0f,	0.0f, 0.0f, 1.0f,	1.0f,  0.0f, // bottom right vertex
		-1.0f, -1.0f,   0.0f,	0.0f, 0.0f, 1.0f,	0.0f,  0.0f, // bottom left vertex

		// Book
		// Front Face			// Plane Normal		// Texture Coords
		-1.0f, -1.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.15f,  0.5f, // bottom left vertex
		-1.0f,  0.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.15f, 0.94f, // top left vertex
		-0.5f,  0.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.83f, 0.94f, // top right vertex
		-0.5f,  0.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.83f, 0.94f, // top right vertex
		-0.5f, -1.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.83f,  0.5f, // bottom right vertex
		-1.0f, -1.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.15f,  0.5f, // bottom left vertex
		// Back Face
		-1.0f, -1.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.15f,  0.0f, // bottom left vertex
		-1.0f,  0.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.15f, 0.44f, // top left vertex
		-0.5f,  0.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.83f, 0.44f, // top right vertex
		-0.5f,  0.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.83f, 0.44f, // top right vertex
		-0.5f, -1.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.83f,  0.0f, // bottom right vertex
		-1.0f, -1.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.15f,  0.0f, // bottom left vertex
		// Left Face
		-1.0f, -1.0f,   0.1f,  -1.0f, 0.0f, 0.0f,	0.15f,  0.5f, // bottom left vertex
		-1.0f,  0.0f,   0.1f,  -1.0f, 0.0f, 0.0f,	0.15f, 0.94f, // top left vertex
		-1.0f,  0.0f, 0.001f,  -1.0f, 0.0f, 0.0f,	0.0f,  0.94f, // back top left vertex
		-1.0f,  0.0f, 0.001f,  -1.0f, 0.0f, 0.0f,	0.0f,  0.94f, // back top left vertex
		-1.0f, -1.0f, 0.001f,  -1.0f, 0.0f, 0.0f,	0.0f,   0.5f, // back bottom left vertex
		-1.0f, -1.0f,   0.1f,  -1.0f, 0.0f, 0.0f,	0.15f,  0.5f, // bottom left vertex
		// Right Face
		-0.5f, -1.0f,   0.1f,   1.0f, 0.0f, 0.0f,	1.0f,  0.5f, // bottom right vertex
		-0.5f,  0.0f,   0.1f,   1.0f, 0.0f, 0.0f,   1.0f, 0.94f, // top right vertex
		-0.5f,  0.0f, 0.001f,   1.0f, 0.0f, 0.0f,	0.83f, 0.94f, // back top right vertex
		-0.5f,  0.0f, 0.001f,   1.0f, 0.0f, 0.0f,	0.83f, 0.94f, // back top right vertex
		-0.5f, -1.0f, 0.001f,   1.0f, 0.0f, 0.0f,	0.83f,  0.5f, // back bottom right vertex
		-0.5f, -1.0f,   0.1f,   1.0f, 0.0f, 0.0f,	1.0f,  0.5f, // bottom right vertex
		// Top Face
		-1.0f,  0.0f,   0.1f,	0.0f, 1.0f, 0.0f,   0.15f, 0.94f, // top left vertex
		-1.0f,  0.0f, 0.001f,	0.0f, 1.0f, 0.0f,	0.15f,  1.0f, // back top left vertex
		-0.5f,  0.0f, 0.001f,	0.0f, 1.0f, 0.0f,	0.83f,  1.0f, // back top right vertex
		-0.5f,  0.0f, 0.001f,	0.0f, 1.0f, 0.0f,	0.83f,  1.0f, // back top right vertex
		-0.5f,  0.0f,   0.1f,	0.0f, 1.0f, 0.0f,	0.83f, 0.94f, // top right vertex
		-1.0f,  0.0f,   0.1f,	0.0f, 1.0f, 0.0f,	0.15f, 0.94f, // top left vertex
		// Bottom Face
		-1.0f, -1.0f,   0.1f,	0.0f,-1.0f, 0.0f,	 0.15f,  0.5f, // bottom left vertex
		-1.0f, -1.0f, 0.001f,	0.0f,-1.0f, 0.0f,	 0.15f, 0.44f, // back bottom left vertex
		-0.5f, -1.0f, 0.001f,	0.0f,-1.0f, 0.0f,	 0.83f, 0.44f, // back bottom right vertex
		-0.5f, -1.0f, 0.001f,	0.0f,-1.0f, 0.0f,	 0.83f, 0.44f, // back bottom right vertex
		-0.5f, -1.0f,   0.1f,	0.0f,-1.0f, 0.0f,    0.83f,  0.5f, // bottom right vertex
		-1.0f, -1.0f,   0.1f,	0.0f,-1.0f, 0.0f,    0.15f,  0.5f, // bottom left vertex

		// Rubik's Cube
		// Front Face
		-0.75f,-0.25f,0.351f,	0.0f, 0.0f, 1.0f,	 0.34f,  0.5f, // bottom left vertex
		-0.75f, 0.0f, 0.351f,	0.0f, 0.0f, 1.0f,	 0.34f, 0.75f, // top left vertex
		-0.5f,  0.0f, 0.351f,	0.0f, 0.0f, 1.0f,	 0.66f, 0.75f, // top right vertex
		-0.5f,  0.0f, 0.351f,	0.0f, 0.0f, 1.0f,	 0.66f, 0.75f, // top right vertex
		-0.5f, -0.25f,0.351f,	0.0f, 0.0f, 1.0f,	 0.66f,  0.5f, // bottom right vertex
		-0.75f,-0.25f,0.351f,	0.0f, 0.0f, 1.0f,	 0.34f,  0.5f, // bottom left vertex
		// Back Face
		-0.75f,-0.25f,0.101f,	0.0f, 0.0f,-1.0f,	 0.34f,  0.0f, // back bottom left vertex
		-0.75f, 0.0f, 0.101f,	0.0f, 0.0f,-1.0f,	 0.34f, 0.25f, // back top left vertex
		-0.5f,  0.0f, 0.101f,	0.0f, 0.0f,-1.0f,   0.665f, 0.25f, // back top right vertex
		-0.5f,  0.0f, 0.101f,	0.0f, 0.0f,-1.0f,	0.665f, 0.25f, // back top right vertex
		-0.5f, -0.25f,0.101f,	0.0f, 0.0f,-1.0f,	0.665f,  0.0f, // back bottom right vertex
		-0.75f,-0.25f,0.101f,	0.0f, 0.0f,-1.0f,	 0.34f,  0.0f, // back bottom left vertex
		// Left Face
		-0.75f,-0.25f,0.351f,  -1.0f, 0.0f, 0.0f,	 0.33f,  0.5f, // bottom left vertex
		-0.75f, 0.0f, 0.351f,  -1.0f, 0.0f, 0.0f,	 0.33f, 0.75f, // top left vertex
		-0.75f, 0.0f, 0.101f,  -1.0f, 0.0f, 0.0f,	  0.0f, 0.75f, // back top left vertex
		-0.75f, 0.0f, 0.101f,  -1.0f, 0.0f, 0.0f,	  0.0f, 0.75f, // back top left vertex
		-0.75f,-0.25f,0.101f,  -1.0f, 0.0f, 0.0f,	  0.0f,  0.5f, // back bottom left vertex
		-0.75f,-0.25f,0.351f,  -1.0f, 0.0f, 0.0f,	 0.33f,  0.5f, // bottom left vertex
		// Right Face
		-0.5f, -0.25f,0.351f,   1.0f, 0.0f, 0.0f,	 0.66f,  0.5f, // bottom right vertex
		-0.5f,  0.0f, 0.351f,   1.0f, 0.0f, 0.0f,	 0.66f, 0.75f, // top right vertex
		-0.5f,  0.0f, 0.101f,   1.0f, 0.0f, 0.0f,	  1.0f, 0.75f, // back top right vertex
		-0.5f,  0.0f, 0.101f,   1.0f, 0.0f, 0.0f,	  1.0f, 0.75f, // back top right vertex
		-0.5f, -0.25f,0.101f,   1.0f, 0.0f, 0.0f,	  1.0f,  0.5f, // back bottom right vertex
		-0.5f, -0.25f,0.351f,   1.0f, 0.0f, 0.0f,	 0.66f,  0.5f, // bottom right vertex
		// Top Face
		-0.75f, 0.0f, 0.351f,	0.0f, 1.0f, 0.0f,	 0.34f, 0.75f, // top left vertex
		-0.75f, 0.0f, 0.101f,	0.0f, 1.0f, 0.0f,	 0.34f,  1.0f, // back top left vertex
		-0.5f,  0.0f, 0.101f,	0.0f, 1.0f, 0.0f,	0.665f,  1.0f, // back top right vertex
		-0.5f,  0.0f, 0.101f,	0.0f, 1.0f, 0.0f,	0.665f,  1.0f, // back top right vertex
		-0.5f,  0.0f, 0.351f,	0.0f, 1.0f, 0.0f,	0.665f, 0.75f, // top right vertex
		-0.75f, 0.0f, 0.351f,	0.0f, 1.0f, 0.0f,	 0.34f, 0.75f, // top left vertex
		// Bottom Face
		-0.75f,-0.25f,0.351f,	0.0f,-1.0f, 0.0f,	 0.34f,  0.5f, // bottom left vertex
		-0.75f,-0.25f,0.101f,	0.0f,-1.0f, 0.0f,	 0.34f, 0.25f, // back bottom left vertex
		-0.5f, -0.25f,0.101f,	0.0f,-1.0f, 0.0f,	0.665f, 0.25f, // back bottom right vertex
		-0.5f, -0.25f,0.101f,	0.0f,-1.0f, 0.0f,	0.665f, 0.25f, // back bottom right vertex
		-0.5f, -0.25f,0.351f,	0.0f,-1.0f, 0.0f,	0.665f,  0.5f, // bottom right vertex
		-0.75f,-0.25f,0.351f,	0.0f,-1.0f, 0.0f,	 0.34f,  0.5f, // bottom left vertex

		// Perfume Bottle
		// Front Face			// Plane Normal		// Texture Coords
		-1.0f, -1.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.15f,  0.5f, // bottom left vertex
		-1.0f,  0.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.15f, 0.94f, // top left vertex
		-0.5f,  0.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.83f, 0.94f, // top right vertex
		-0.5f,  0.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.83f, 0.94f, // top right vertex
		-0.5f, -1.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.83f,  0.5f, // bottom right vertex
		-1.0f, -1.0f,   0.1f,	0.0f, 0.0f, 1.0f,	0.15f,  0.5f, // bottom left vertex
		// Back Face
		-1.0f, -1.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.15f,  0.0f, // bottom left vertex
		-1.0f,  0.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.15f, 0.44f, // top left vertex
		-0.5f,  0.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.83f, 0.44f, // top right vertex
		-0.5f,  0.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.83f, 0.44f, // top right vertex
		-0.5f, -1.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.83f,  0.0f, // bottom right vertex
		-1.0f, -1.0f, 0.001f,	0.0f, 0.0f,-1.0f,   0.15f,  0.0f, // bottom left vertex
		// Left Face
		-1.0f, -1.0f,   0.1f,  -1.0f, 0.0f, 0.0f,	0.15f,  0.5f, // bottom left vertex
		-1.0f,  0.0f,   0.1f,  -1.0f, 0.0f, 0.0f,	0.15f, 0.94f, // top left vertex
		-1.0f,  0.0f, 0.001f,  -1.0f, 0.0f, 0.0f,	0.0f,  0.94f, // back top left vertex
		-1.0f,  0.0f, 0.001f,  -1.0f, 0.0f, 0.0f,	0.0f,  0.94f, // back top left vertex
		-1.0f, -1.0f, 0.001f,  -1.0f, 0.0f, 0.0f,	0.0f,   0.5f, // back bottom left vertex
		-1.0f, -1.0f,   0.1f,  -1.0f, 0.0f, 0.0f,	0.15f,  0.5f, // bottom left vertex
		// Right Face
		-0.5f, -1.0f,   0.1f,   1.0f, 0.0f, 0.0f,	1.0f,  0.5f, // bottom right vertex
		-0.5f,  0.0f,   0.1f,   1.0f, 0.0f, 0.0f,   1.0f, 0.94f, // top right vertex
		-0.5f,  0.0f, 0.001f,   1.0f, 0.0f, 0.0f,	0.83f, 0.94f, // back top right vertex
		-0.5f,  0.0f, 0.001f,   1.0f, 0.0f, 0.0f,	0.83f, 0.94f, // back top right vertex
		-0.5f, -1.0f, 0.001f,   1.0f, 0.0f, 0.0f,	0.83f,  0.5f, // back bottom right vertex
		-0.5f, -1.0f,   0.1f,   1.0f, 0.0f, 0.0f,	1.0f,  0.5f, // bottom right vertex
		// Top Face
		-1.0f,  0.0f,   0.1f,	0.0f, 1.0f, 0.0f,   0.15f, 0.94f, // top left vertex
		-1.0f,  0.0f, 0.001f,	0.0f, 1.0f, 0.0f,	0.15f,  1.0f, // back top left vertex
		-0.5f,  0.0f, 0.001f,	0.0f, 1.0f, 0.0f,	0.83f,  1.0f, // back top right vertex
		-0.5f,  0.0f, 0.001f,	0.0f, 1.0f, 0.0f,	0.83f,  1.0f, // back top right vertex
		-0.5f,  0.0f,   0.1f,	0.0f, 1.0f, 0.0f,	0.83f, 0.94f, // top right vertex
		-1.0f,  0.0f,   0.1f,	0.0f, 1.0f, 0.0f,	0.15f, 0.94f, // top left vertex
		// Bottom Face
		-1.0f, -1.0f,   0.1f,	0.0f,-1.0f, 0.0f,	 0.15f,  0.5f, // bottom left vertex
		-1.0f, -1.0f, 0.001f,	0.0f,-1.0f, 0.0f,	 0.15f, 0.44f, // back bottom left vertex
		-0.5f, -1.0f, 0.001f,	0.0f,-1.0f, 0.0f,	 0.83f, 0.44f, // back bottom right vertex
		-0.5f, -1.0f, 0.001f,	0.0f,-1.0f, 0.0f,	 0.83f, 0.44f, // back bottom right vertex
		-0.5f, -1.0f,   0.1f,	0.0f,-1.0f, 0.0f,    0.83f,  0.5f, // bottom right vertex
		-1.0f, -1.0f,   0.1f,	0.0f,-1.0f, 0.0f,    0.15f,  0.5f, // bottom left vertex

		// Perfume Cap
		// Front Face			// Plane Normal		// Texture Coords
		-0.5f, 0.0f, 0.1f,		0.0f, 0.0f, 1.0f,	0.15f, 0.5f, // bottom left vertex
		-0.5f, 0.5f, 0.1f,		0.0f, 0.0f, 1.0f,	0.15f, 0.94f, // top left vertex
		 0.5f, 0.5f, 0.1f,		0.0f, 0.0f, 1.0f,	0.83f, 0.94f, // top right vertex
		 0.5f, 0.5f, 0.1f,		0.0f, 0.0f, 1.0f,	0.83f, 0.94f, // top right vertex
		 0.5f, 0.0f, 0.1f,		0.0f, 0.0f, 1.0f,	0.83f, 0.5f, // bottom right vertex
		-0.5f, 0.0f, 0.1f,		0.0f, 0.0f, 1.0f,	0.15f, 0.5f, // bottom left vertex
		// Back Face
		-0.5f, 0.0f, 0.001f,	0.0f, 0.0f, -1.0f,	0.15f, 0.0f, // bottom left vertex
		-0.5f, 0.5f, 0.001f,	0.0f, 0.0f, -1.0f,	0.15f, 0.44f, // top left vertex
		 0.5f, 0.5f, 0.001f,	0.0f, 0.0f, -1.0f,	0.83f, 0.44f, // top right vertex
		 0.5f, 0.5f, 0.001f,	0.0f, 0.0f, -1.0f,	0.83f, 0.44f, // top right vertex
		 0.5f, 0.0f, 0.001f,	0.0f, 0.0f, -1.0f,	0.83f, 0.0f, // bottom right vertex
		-0.5f, 0.0f, 0.001f,	0.0f, 0.0f, -1.0f,	0.15f, 0.0f, // bottom left vertex
		// Left Face
		-0.5f, 0.0f, 0.1f,		-1.0f, 0.0f, 0.0f,	0.15f, 0.5f, // bottom left vertex
		-0.5f, 0.5f, 0.1f,		-1.0f, 0.0f, 0.0f,  0.15f, 0.94f, // top left vertex
		-0.5f, 0.5f, 0.001f,	-1.0f, 0.0f, 0.0f,	0.0f, 0.94f, // back top left vertex
		-0.5f, 0.5f, 0.001f,	-1.0f, 0.0f, 0.0f,	0.0f, 0.94f, // back top left vertex
		-0.5f, 0.0f, 0.001f,	-1.0f, 0.0f, 0.0f,	0.0f, 0.5f, // back bottom left vertex
		-0.5f, 0.0f, 0.1f,		-1.0f, 0.0f, 0.0f,	0.15f, 0.5f, // bottom left vertex
		// Right Face
		 0.5f, 0.0f, 0.1f,		1.0f, 0.0f, 0.0f,	1.0f, 0.5f, // bottom right vertex
		 0.5f, 0.5f, 0.1f,		1.0f, 0.0f, 0.0f,	1.0f, 0.94f, // top right vertex
		 0.5f, 0.5f, 0.001f,	1.0f, 0.0f, 0.0f,	0.83f, 0.94f, // back top right vertex
		 0.5f, 0.5f, 0.001f,	1.0f, 0.0f, 0.0f,	0.83f, 0.94f, // back top right vertex
		 0.5f, 0.0f, 0.001f,	1.0f, 0.0f, 0.0f,	0.83f, 0.5f, // back bottom right vertex
		 0.5f, 0.0f, 0.1f,		1.0f, 0.0f, 0.0f,	1.0f, 0.5f, // bottom right vertex
		// Top Face
		-0.5f, 0.5f, 0.1f,		0.0f, 1.0f, 0.0f,	0.15f, 0.94f, // top left vertex
		-0.5f, 0.5f, 0.001f,	0.0f, 1.0f, 0.0f,	0.15f, 1.0f, // back top left vertex
		 0.5f, 0.5f, 0.001f,	0.0f, 1.0f, 0.0f,	0.83f, 1.0f, // back top right vertex
		 0.5f, 0.5f, 0.001f,	0.0f, 1.0f, 0.0f,	0.83f, 1.0f, // back top right vertex
		 0.5f, 0.5f, 0.1f,		0.0f, 1.0f, 0.0f,	0.83f, 0.94f, // top right vertex
		-0.5f, 0.5f, 0.1f,		0.0f, 1.0f, 0.0f,	0.15f, 0.94f, // top left vertex
		// Bottom Face
		-0.5f, 0.0f, 0.1f,		0.0f, -1.0f, 0.0f,	0.15f, 0.5f, // bottom left vertex
		-0.5f, 0.0f, 0.001f,	0.0f, -1.0f, 0.0f,	0.15f, 0.44f, // back bottom left vertex
		 0.5f, 0.0f, 0.001f,	0.0f, -1.0f, 0.0f,	0.83f, 0.44f, // back bottom right vertex
		 0.5f, 0.0f, 0.001f,	0.0f, -1.0f, 0.0f,	0.83f, 0.44f, // back bottom right vertex
		 0.5f, 0.0f, 0.1f,		0.0f, -1.0f, 0.0f,	0.83f, 0.5f, // bottom right vertex
		-0.5f, 0.0f, 0.1f,		0.0f, -1.0f, 0.0f,	0.15f, 0.5f, // bottom left vertex

	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	// generating vertex array object names
	glGenVertexArrays(1, &mesh.vaos[0]);
	// Binding generated vertex array name
	glBindVertexArray(mesh.vaos[0]);

	// Generates buffers for vertex data and indices
	glGenBuffers(1, &mesh.vbos[0]);
	// Binding vertex data
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	// Sending vertex/coordinate data to GPU
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	// Setting stride, which is 5 (3 + 2)
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	// Creating vertex attrib pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);

	// Cylinder VAO
	glUseProgram(gCylProgramId);

	glGenVertexArrays(1, &mesh.vaos[1]);
	glBindVertexArray(mesh.vaos[1]);
	glGenBuffers(1, &mesh.vbos[1]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ARRAY_BUFFER, cylinder1.getInterleavedVertexSize(), cylinder1.getInterleavedVertices(), GL_STATIC_DRAW);

	// Creating vertex attrib pointers
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, cylinder1.getInterleavedStride(), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, cylinder1.getInterleavedStride(), (void*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, cylinder1.getInterleavedStride(), (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);

	// Marble Texture
	const char* texFilename = "../CS330 Final Project/Resources/Textures/marble.jfif";
	if (!UCreateTexture(texFilename, texture0))
	{
		cout << "Failed to load texture " << texFilename << endl;
	}
	// Book Texture
	texFilename = "../CS330 Final Project/Resources/Textures/gulagArchipelago.png";
	if (!UCreateTexture(texFilename, texture1))
	{
		cout << "Failed to load texture " << texFilename << endl;
	}
	// Rubik's Cube Texture
	texFilename = "../CS330 Final Project/Resources/Textures/rubikscube.png";
	if (!UCreateTexture(texFilename, texture2))
	{
		cout << "Failed to load texture " << texFilename << endl;
	}
	// Dust Texture
	texFilename = "../CS330 Final Project/Resources/Textures/dust.jpg";
	if (!UCreateTexture(texFilename, texture3))
	{
		cout << "Failed to load texture " << texFilename << endl;
	}
}

void UDestroyMesh(GLMesh& mesh)
{
	// Delete mesh
	glDeleteVertexArrays(1, &mesh.vaos[0]);
	glDeleteVertexArrays(1, &mesh.vaos[1]);
	glDeleteBuffers(1, &mesh.vbos[0]);
	glDeleteBuffers(1, &mesh.vbos[1]);
}

bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
	// for comp and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Creating shader program object
	programId = glCreateProgram();

	// Creating vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// get shader sources
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// compile vertex shader
	glCompileShader(vertexShaderId);
	// check for compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// compile fragment shader
	glCompileShader(fragmentShaderId);
	// check for compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attach shaders to shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId); // link shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);

	return true;
}

void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	// modify if mouse sensitivity is too low or high
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}

void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}

bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);

	if (image)
	{
		flipImageVertically(image, width, height, channels);

		// generates texture names
		glGenTextures(1, &textureId);
		// binding texure to 2D texture
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set texture wrapping params
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering params
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Assigning texture to pointer, and defining how it will be stored in memory
		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << "channels" << endl;
			return false;
		}

		// generating mipmap for GL_TEXTURE_2D
		glGenerateMipmap(GL_TEXTURE_2D);

		// free loaded image
		stbi_image_free(image);
		// rebinding GL_TEXTURE_2D to nothing
		glBindTexture(GL_TEXTURE_2D, 0);

		return true;
	}

	return false;
}

void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}