#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <render/shader.h>

#include "skybox.cpp"  
#include "terrain.cpp"
#include "model.cpp"
#include "forest.cpp"  
#include "glowing_orbs.cpp"
#include "owl.cpp"

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

static GLFWwindow *window;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

static glm::vec3 eye_center;
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);

static glm::vec3 moonDirection = glm::normalize(glm::vec3(0.5f, 1.0f, -0.5f));

glm::vec3 moonDir = glm::normalize(glm::vec3(0.5f, 1.0f, -0.5f));
glm::vec3 moonColor = glm::vec3(0.8f, 0.8f, 1.0f);
glm::vec3 nightAmbient = glm::vec3(0.1f, 0.1f, 0.2f);

static double fpsLastTime = glfwGetTime();
static int frameCount = 0;


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

    TerrainManager terrainManager;
    terrainManager.init(128, 11);

    Forest forest; 

    GLuint treeShaderID = LoadShadersFromFile(
        "C:/Users/User/Desktop/graphics/final proj/project/tree.vert",
        "C:/Users/User/Desktop/graphics/final proj/project/tree.frag"
    );

    Model tree1;
    tree1.init("C:/Users/User/Desktop/graphics/final proj/project/assets/Elm tree/ElmTree.OBJ",
               "C:/Users/User/Desktop/graphics/final proj/project/assets/Elm tree/ElmTree_BaseColor.png",
               treeShaderID);

    Model tree2;
    tree2.init("C:/Users/User/Desktop/graphics/final proj/project/assets/Bubinga/BubingaTree.OBJ",
               "C:/Users/User/Desktop/graphics/final proj/project/assets/Bubinga/BubingaTree_BaseColor.png",
               treeShaderID);

    Owl owl;
    owl.initialize();
    float owlAnimTime = 0.0f;
    
    GLuint orbShaderID = LoadShadersFromFile(
        "C:/Users/User/Desktop/graphics/final proj/project/orb.vert",
        "C:/Users/User/Desktop/graphics/final proj/project/orb.frag"
    );

    OrbSystem orbs;
    orbs.init();
    orbs.regenerateForVisibleChunks(terrainManager);

    float lastTime = glfwGetTime();
    eye_center = glm::vec3(0, 5, 50);
    lookat = glm::vec3(0, 5, 0);

    glm::mat4 viewMatrix, projectionMatrix;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    projectionMatrix = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 3000.0f);

    GLint viewLoc = glGetUniformLocation(treeShaderID, "view");
    GLint projLoc = glGetUniformLocation(treeShaderID, "projection");

    GLuint moonShaderID = LoadShadersFromFile(
        "C:/Users/User/Desktop/graphics/final proj/project/moon.vert",
        "C:/Users/User/Desktop/graphics/final proj/project/moon.frag"
    );

    float moonVertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f };
    GLuint moonVAO, moonVBO;
    glGenVertexArrays(1, &moonVAO);
    glGenBuffers(1, &moonVBO);
    glBindVertexArray(moonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, moonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(moonVertices), moonVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glm::vec3 moonPosition = glm::vec3(0.0f, 300.0f, -1000.0f);
    moonDir = glm::normalize(moonPosition);

    // CREATE SEPARATE VBOs FOR EACH TREE TYPE
    GLuint instanceVBO_tree1, instanceVBO_tree2;
    glGenBuffers(1, &instanceVBO_tree1);
    glGenBuffers(1, &instanceVBO_tree2);

    do {
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
        lastTime = currentTime;
    frameCount++;
if (currentTime - fpsLastTime >= 0.5) {  // Update twice per second
    double fps = frameCount / (currentTime - fpsLastTime);
    std::string title = "Moonlit Forest - FPS: " + std::to_string((int)fps);
    glfwSetWindowTitle(window, title.c_str());
    frameCount = 0;
    fpsLastTime = currentTime;
}

        
        float groundY = terrainManager.chunks[0].getHeight(eye_center.x, eye_center.z) - 10.0f;
        if (eye_center.y < groundY + 2.0f) eye_center.y = groundY + 2.0f;

        float celestialDistance = 1500.0f;
        glm::vec3 dynamicMoonPos = eye_center + (moonDirection * celestialDistance);

        terrainManager.update(eye_center);
        viewMatrix = glm::lookAt(eye_center, lookat, up);
        orbs.update(glfwGetTime());

        static glm::vec2 lastPos(-9999);
        if (glm::distance(glm::vec2(eye_center.x, eye_center.z), lastPos) > 10.0f) {
            forest.regenerateForVisibleChunks(terrainManager);
            orbs.regenerateForVisibleChunks(terrainManager);
            lastPos = glm::vec2(eye_center.x, eye_center.z);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        skybox.render(viewMatrix, projectionMatrix);
        terrainManager.render(viewMatrix, projectionMatrix, eye_center);

        owlAnimTime += deltaTime;
        glm::mat4 owlTransform = glm::mat4(1.0f);
        owlTransform = glm::translate(owlTransform, glm::vec3(0, 15, -20));
        owlTransform = glm::scale(owlTransform, glm::vec3(1.0f));
        owl.render(projectionMatrix * viewMatrix * owlTransform);

        glUseProgram(moonShaderID);
        glDisable(GL_DEPTH_TEST);
        glUniformMatrix4fv(glGetUniformLocation(moonShaderID, "projection"), 1, GL_FALSE, &projectionMatrix[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(moonShaderID, "view"), 1, GL_FALSE, &viewMatrix[0][0]);
        glUniform3fv(glGetUniformLocation(moonShaderID, "moonPos"), 1, &moonPosition[0]);
        glUniform1f(glGetUniformLocation(moonShaderID, "moonSize"), 60.0f);
        glBindVertexArray(moonVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glEnable(GL_DEPTH_TEST);

        std::vector<glm::mat4> type0Trees, type1Trees;
        for (const auto& tree : forest.trees) {
            if (tree.type == 0) {
                type0Trees.push_back(tree.modelMatrix);
            } else {
                type1Trees.push_back(tree.modelMatrix);
            }
        }

        glUseProgram(treeShaderID);
        glUniform3fv(glGetUniformLocation(treeShaderID, "lightDir"), 1, &moonDir[0]);
        glUniform3fv(glGetUniformLocation(treeShaderID, "lightColor"), 1, &moonColor[0]);
        glUniform3fv(glGetUniformLocation(treeShaderID, "ambientColor"), 1, &nightAmbient[0]);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewMatrix[0][0]);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projectionMatrix[0][0]);

        if (!type0Trees.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_tree1);
            glBufferData(GL_ARRAY_BUFFER, type0Trees.size() * sizeof(glm::mat4), type0Trees.data(), GL_DYNAMIC_DRAW);
            
            glBindVertexArray(tree1.VAO);
            for (int i = 0; i < 4; i++) {
                glEnableVertexAttribArray(3 + i);
                glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
                glVertexAttribDivisor(3 + i, 1);
            }
            
            glBindTexture(GL_TEXTURE_2D, tree1.textureID);
            glDrawArraysInstanced(GL_TRIANGLES, 0, tree1.vertexCount, (GLsizei)type0Trees.size());
        }

        if (!type1Trees.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_tree2);
            glBufferData(GL_ARRAY_BUFFER, type1Trees.size() * sizeof(glm::mat4), type1Trees.data(), GL_DYNAMIC_DRAW);
            
            glBindVertexArray(tree2.VAO);
            for (int i = 0; i < 4; i++) {
                glEnableVertexAttribArray(3 + i);
                glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
                glVertexAttribDivisor(3 + i, 1);
            }
            
            glBindTexture(GL_TEXTURE_2D, tree2.textureID);
            glDrawArraysInstanced(GL_TRIANGLES, 0, tree2.vertexCount, (GLsizei)type1Trees.size());
        }

        orbs.render(orbShaderID, viewMatrix, projectionMatrix);
        glBindVertexArray(0);
        glfwSwapBuffers(window);
        glfwPollEvents();

    } while (!glfwWindowShouldClose(window));

    glDeleteBuffers(1, &instanceVBO_tree1);
    glDeleteBuffers(1, &instanceVBO_tree2);
    skybox.cleanup();
    terrainManager.cleanup();
    forest.cleanup();
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        eye_center = glm::vec3(0, 30, 50);
        lookat = glm::vec3(0, 10, 0);
    }

    if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 forward = glm::normalize(lookat - eye_center);
        eye_center += forward * 2.0f;
        lookat += forward * 2.0f;
    }

    if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 forward = glm::normalize(lookat - eye_center);
        eye_center -= forward * 2.0f;
        lookat -= forward * 2.0f;
    }

    if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 forward = glm::normalize(lookat - eye_center);
        glm::vec3 right = glm::normalize(glm::cross(forward, up));
        eye_center -= right * 2.0f;
        lookat -= right * 2.0f;
    }

    if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 forward = glm::normalize(lookat - eye_center);
        glm::vec3 right = glm::normalize(glm::cross(forward, up));
        eye_center += right * 2.0f;
        lookat += right * 2.0f;
    }

    if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 direction = lookat - eye_center;
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), 0.05f, glm::cross(direction, up));
        lookat = eye_center + glm::vec3(rotation * glm::vec4(direction, 0.0f));
    }

    if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 direction = lookat - eye_center;
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), -0.05f, glm::cross(direction, up));
        lookat = eye_center + glm::vec3(rotation * glm::vec4(direction, 0.0f));
    }

    if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 direction = lookat - eye_center;
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), 0.05f, up);
        lookat = eye_center + glm::vec3(rotation * glm::vec4(direction, 0.0f));
    }

    if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        glm::vec3 direction = lookat - eye_center;
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), -0.05f, up);
        lookat = eye_center + glm::vec3(rotation * glm::vec4(direction, 0.0f));
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}