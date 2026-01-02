#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <render/shader.h>
#include "skybox.cpp"  
#include "terrain.cpp"
#include "model.cpp"
#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>


static GLFWwindow *window;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// OpenGL camera view parameters
static glm::vec3 eye_center;
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);

// View control 
static float viewAzimuth = 0.f;
static float viewPolar = 0.f;
static float viewDistance = 10.0f;


int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

// Windowed mode at monitor resolution
window = glfwCreateWindow(mode->width, mode->height, "Moonlit Forest", NULL, NULL);


	if (window == NULL)
	{
		std::cerr << "Failed to open a GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetKeyCallback(window, key_callback);

	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}

	glClearColor(0.2f, 0.2f, 0.25f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Initialize skybox
	Skybox skybox;
		std::vector<std::string> skyboxFaces {
    "C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/px.png",
    "C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/nx.png",
    "C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/py.png",
    "C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/ny.png",
    "C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/pz.png",
    "C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/nz.png"
};
	skybox.initialize(skyboxFaces);

Terrain terrain;
terrain.init(128);

GLuint treeShaderID = LoadShadersFromFile(
    "C:/Users/User/Desktop/graphics/final proj/project/tree.vert",
    "C:/Users/User/Desktop/graphics/final proj/project/tree.frag"
);

Model tree;
tree.init(
    "C:/Users/User/Desktop/graphics/final proj/project/assets/Elm tree/ElmTree.OBJ",
    "C:/Users/User/Desktop/graphics/final proj/project/assets/Elm tree/ElmTree_BaseColor.png",
    treeShaderID
);

	// Camera setup
    eye_center.y = viewDistance * cos(viewPolar)+50.0f;
    eye_center.x = viewDistance * cos(viewAzimuth);
    eye_center.z = viewDistance * sin(viewAzimuth);

	glm::mat4 viewMatrix, projectionMatrix;
    glm::float32 FoV = 45;
	glm::float32 zNear = 0.1f; 
	glm::float32 zFar = 3000.0f;
	int width, height;
glfwGetFramebufferSize(window, &width, &height);
projectionMatrix = glm::perspective(glm::radians(FoV), (float)width / (float)height, zNear, zFar);

	do
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		viewMatrix = glm::lookAt(eye_center, lookat, up);
		skybox.render(viewMatrix, projectionMatrix);
		glm::mat4 vp = projectionMatrix * viewMatrix;
         glm::mat4 modelMatrix = glm::mat4(1.0f);
   modelMatrix = glm::translate(modelMatrix, glm::vec3(-32, -10, -32));
    glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	terrain.render(mvp);

	glm::mat4 treeModel = glm::mat4(1.0f);
treeModel = glm::translate(treeModel, glm::vec3(0, -10, 0)); 
treeModel = glm::scale(treeModel, glm::vec3(0.6f, 0.6f, 0.6f)); 
glm::mat4 treeMVP = projectionMatrix * viewMatrix * treeModel;
tree.render(treeMVP);
		glfwSwapBuffers(window);
		glfwPollEvents();

	} while (!glfwWindowShouldClose(window));

	// Cleanup
	skybox.cleanup();
    terrain.cleanup();

	glfwTerminate();
	return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		// Reset to initial position
		eye_center = glm::vec3(0, 40, 50);
		lookat = glm::vec3(0, 0, 0);
		std::cout << "Reset." << std::endl;
	}

	// WASD movement
	if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		glm::vec3 forward = glm::normalize(lookat - eye_center);
		eye_center += forward * 2.0f;
		lookat += forward * 2.0f;
	}

	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		glm::vec3 forward = glm::normalize(lookat - eye_center);
		eye_center -= forward * 2.0f;
		lookat -= forward * 2.0f;
	}

	if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		glm::vec3 forward = glm::normalize(lookat - eye_center);
		glm::vec3 right = glm::normalize(glm::cross(forward, up));
		eye_center -= right * 2.0f;
		lookat -= right * 2.0f;
	}

	if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		glm::vec3 forward = glm::normalize(lookat - eye_center);
		glm::vec3 right = glm::normalize(glm::cross(forward, up));
		eye_center += right * 2.0f;
		lookat += right * 2.0f;
	}

	// Arrow keys for looking around
	if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		// Look up
		glm::vec3 direction = lookat - eye_center;
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), 0.05f, glm::cross(direction, up));
		lookat = eye_center + glm::vec3(rotation * glm::vec4(direction, 0.0f));
	}

	if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		// Look down
		glm::vec3 direction = lookat - eye_center;
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), -0.05f, glm::cross(direction, up));
		lookat = eye_center + glm::vec3(rotation * glm::vec4(direction, 0.0f));
	}

	if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		// Look left
		glm::vec3 direction = lookat - eye_center;
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), 0.05f, up);
		lookat = eye_center + glm::vec3(rotation * glm::vec4(direction, 0.0f));
	}

	if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		// Look right
		glm::vec3 direction = lookat - eye_center;
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), -0.05f, up);
		lookat = eye_center + glm::vec3(rotation * glm::vec4(direction, 0.0f));
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

