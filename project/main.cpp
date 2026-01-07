#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <render/shader.h>

#include "skybox.cpp"  
#include "terrain.cpp" // Terrain must come BEFORE forest
#include "model.cpp"
#include "forest.cpp"  // Forest can now see TerrainManager
#include "glowing_orbs.cpp"

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

static GLFWwindow *window;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

static glm::vec3 eye_center;
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);

// Near your eye_center and lookat definitions
static glm::vec3 moonDirection = glm::normalize(glm::vec3(0.5f, 1.0f, -0.5f));

glm::vec3 moonDir = glm::normalize(glm::vec3(0.5f, 1.0f, -0.5f)); // High in the sky
glm::vec3 moonColor = glm::vec3(0.8f, 0.8f, 1.0f);               // Pale moonlight
//glm::vec3 moonColor = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 nightAmbient = glm::vec3(0.1f, 0.1f, 0.2f);            // Deep blue shadows

int main(void) {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    window = glfwCreateWindow(mode->width, mode->height, "Moonlit Forest", NULL, NULL);

    if (window == NULL) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    gladLoadGL(glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // 1. Initialize Managers
    Skybox skybox;
    std::vector<std::string> skyboxFaces {"C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/px.png", "C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/nx.png", "C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/py.png", "C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/ny.png", "C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/pz.png", "C:/Users/User/Desktop/graphics/final proj/project/sky_18_cubemap_2k/nz.png"};
    skybox.initialize(skyboxFaces);

    TerrainManager terrainManager;
    terrainManager.init(128, 11);

    // CREATE FOREST INSTANCE HERE 
    Forest forest; 

    // 2. Setup Shaders and Models 
    GLuint treeShaderID = LoadShadersFromFile(
        "C:/Users/User/Desktop/graphics/final proj/project/tree.vert",
        "C:/Users/User/Desktop/graphics/final proj/project/tree.frag"
    );

    Model tree1;
    tree1.init("C:/Users/User/Desktop/graphics/final proj/project/assets/Elm tree/ElmTree.OBJ", "C:/Users/User/Desktop/graphics/final proj/project/assets/Elm tree/ElmTree_BaseColor.png", treeShaderID);

    Model tree2;
    tree2.init("C:/Users/User/Desktop/graphics/final proj/project/assets/Bubinga/BubingaTree.OBJ", "C:/Users/User/Desktop/graphics/final proj/project/assets/Bubinga/BubingaTree_BaseColor.png", treeShaderID);

	GLuint orbShaderID = LoadShadersFromFile(
    "C:/Users/User/Desktop/graphics/final proj/project/orb.vert",
    "C:/Users/User/Desktop/graphics/final proj/project/orb.frag"
);

OrbSystem orbs;
orbs.init();
orbs.regenerateForVisibleChunks(terrainManager);

// Track time for animation
float lastTime = glfwGetTime();
    eye_center = glm::vec3(0, 5, 50);
    lookat = glm::vec3(0, 5, 0);

    glm::mat4 viewMatrix, projectionMatrix;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    projectionMatrix = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 3000.0f);

    // Cache uniform locations 
    GLint viewLoc = glGetUniformLocation(treeShaderID, "view");
    GLint projLoc = glGetUniformLocation(treeShaderID, "projection");

	GLuint moonShaderID = LoadShadersFromFile("C:/Users/User/Desktop/graphics/final proj/project/moon.vert", 
												"C:/Users/User/Desktop/graphics/final proj/project/moon.frag");

// Simple Quad for the moon
float moonVertices[] = { -1.0f, -1.0f, 0.0f,  1.0f, -1.0f, 0.0f,  1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f };
GLuint moonVAO, moonVBO;
glGenVertexArrays(1, &moonVAO);
glGenBuffers(1, &moonVBO);
glBindVertexArray(moonVAO);
glBindBuffer(GL_ARRAY_BUFFER, moonVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(moonVertices), moonVertices, GL_STATIC_DRAW);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

// Change the Z to be in the direction you are looking (the -Z axis)
glm::vec3 moonPosition = glm::vec3(0.0f, 300.0f, -1000.0f); 

// Also update your lightDir to match so shadows look correct!
moonDir = glm::normalize(moonPosition);

    do {
        // --- LOGIC ---
        float groundY = terrainManager.chunks[0].getHeight(eye_center.x, eye_center.z) - 10.0f;
        if (eye_center.y < groundY + 2.0f) eye_center.y = groundY + 2.0f;

		float celestialDistance = 1500.0f; // Distance from camera
    glm::vec3 dynamicMoonPos = eye_center + (moonDirection * celestialDistance);

    // Pass this to the moon shader
    glUseProgram(moonShaderID);
    glUniform3fv(glGetUniformLocation(moonShaderID, "moonPos"), 1, &dynamicMoonPos[0]);

        terrainManager.update(eye_center);
        viewMatrix = glm::lookAt(eye_center, lookat, up);

orbs.update(glfwGetTime());
        // Update Forest when moving to new chunks 
        static glm::vec2 lastPos(-9999);
        if (glm::distance(glm::vec2(eye_center.x, eye_center.z), lastPos) > 10.0f) {
            forest.regenerateForVisibleChunks(terrainManager);
            forest.setupInstancedRendering(tree1.VAO); // Link instance data to tree 1 
            forest.setupInstancedRendering(tree2.VAO); // Link instance data to tree 2
			orbs.regenerateForVisibleChunks(terrainManager);  
            lastPos = glm::vec2(eye_center.x, eye_center.z);
        }

        // --- RENDER ---
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        skybox.render(viewMatrix, projectionMatrix);
        terrainManager.render(viewMatrix, projectionMatrix, eye_center);

		glUseProgram(moonShaderID);
        glDisable(GL_DEPTH_TEST); // Ensure moon is drawn over the skybox background
        glUniformMatrix4fv(glGetUniformLocation(moonShaderID, "projection"), 1, GL_FALSE, &projectionMatrix[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(moonShaderID, "view"), 1, GL_FALSE, &viewMatrix[0][0]);
        glUniform3fv(glGetUniformLocation(moonShaderID, "moonPos"), 1, &moonPosition[0]);
        glUniform1f(glGetUniformLocation(moonShaderID, "moonSize"), 60.0f); // Make it big enough to see!

        glBindVertexArray(moonVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glEnable(GL_DEPTH_TEST); // Re-enable depth so trees hide behind terrain/each other

        // --- INSTANCED FOREST RENDER ---
        glUseProgram(treeShaderID);
// Pass the moon light data
glUniform3fv(glGetUniformLocation(treeShaderID, "lightDir"), 1, &moonDir[0]);
glUniform3fv(glGetUniformLocation(treeShaderID, "lightColor"), 1, &moonColor[0]);
glUniform3fv(glGetUniformLocation(treeShaderID, "ambientColor"), 1, &nightAmbient[0]);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewMatrix[0][0]);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projectionMatrix[0][0]);

        // Draw Type 1 (Elm) 
        glBindTexture(GL_TEXTURE_2D, tree1	.textureID);
        glBindVertexArray(tree1.VAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, tree1.vertexCount, (GLsizei)forest.trees.size());

        // Draw Type 2 (Bubinga) 
        glBindTexture(GL_TEXTURE_2D, tree2.textureID);
        glBindVertexArray(tree2.VAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, tree2.vertexCount, (GLsizei)forest.trees.size());
		orbs.render(orbShaderID, viewMatrix, projectionMatrix);
        glBindVertexArray(0);
        glfwSwapBuffers(window);
        glfwPollEvents();

    } while (!glfwWindowShouldClose(window));

    skybox.cleanup();
    terrainManager.cleanup();
    forest.cleanup();
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		// Reset to initial position - looking forward horizontally
		eye_center = glm::vec3(0, 30, 50);
		lookat = glm::vec3(0, 10, 0);
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

