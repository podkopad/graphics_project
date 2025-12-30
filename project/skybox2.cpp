#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <render/shader.h>
#include <stb/stb_image.h>

#include <iostream>

static GLuint LoadSkyboxTexture(const char *texture_file_path) {
    int w, h, channels;
    uint8_t* img = stbi_load(texture_file_path, &w, &h, &channels, 3);
    if (!img) {
        std::cerr << "Failed to load skybox texture: " << texture_file_path << std::endl;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(img);
    
    std::cout << "Loaded skybox2 texture: " << texture_file_path << std::endl;
    return texture;
}

struct Skybox2 {
    GLfloat vertex_buffer_data[72] = {
        // FRONT (+Z)
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,

        // BACK (-Z)
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        // LEFT (-X)
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,

        // RIGHT (+X)
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,

        // TOP (+Y)
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        // BOTTOM (-Y)
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
    };

    GLuint index_buffer_data[36] = {
        0,1,2, 0,2,3,
        4,5,6, 4,6,7,
        8,9,10, 8,10,11,
        12,13,14, 12,14,15,
        16,17,18, 16,18,19,
        20,21,22, 20,22,23
    };

    GLfloat uv_buffer_data[48] = {
        // FRONT (+Z)
        0.751f, 0.665f,  0.999f, 0.665f,  0.999f, 0.334f,  0.751f, 0.334f,
        // BACK (-Z)
        0.251f, 0.665f,  0.499f, 0.665f,  0.499f, 0.334f,  0.251f, 0.334f,
        // LEFT (-X)
        0.001f, 0.665f,  0.249f, 0.665f,  0.249f, 0.334f,  0.001f, 0.334f,
        // RIGHT (+X)
        0.501f, 0.665f,  0.749f, 0.665f,  0.749f, 0.334f,  0.501f, 0.334f,
        // TOP (+Y) 
        0.251f, 0.001f, 0.251f, 0.332f, 0.499f, 0.332f, 0.499f, 0.001f,
        // BOTTOM (-Y)
        0.499f, 0.999f,  0.499f, 0.667f,  0.251f, 0.667f,  0.251f, 0.999f
    };

    GLfloat color_buffer_data[72];

    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint indexBufferID;
    GLuint uvBufferID;
    GLuint colorBufferID;
    GLuint textureID;
    GLuint programID;
    GLuint mvpMatrixID;
    GLuint textureSamplerID;

    void initialize(const char *texturePath) {
        for (int i = 0; i < 72; i++) {
            color_buffer_data[i] = 1.0f;
        }

        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);

        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

        glGenBuffers(1, &colorBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);

        glGenBuffers(1, &uvBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

        glGenBuffers(1, &indexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

        programID = LoadShadersFromFile(
            "C:/Users/User/Desktop/graphics/final proj/project/skybox.vert",
            "C:/Users/User/Desktop/graphics/final proj/project/skybox.frag"
        );

        if (programID == 0)
            std::cerr << "Failed to load skybox2 shaders." << std::endl;

        mvpMatrixID = glGetUniformLocation(programID, "MVP");
        textureSamplerID = glGetUniformLocation(programID, "textureSampler");

        textureID = LoadSkyboxTexture(texturePath);
    }

    void render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
        
        glDepthMask(GL_FALSE);
        glUseProgram(programID);
        glDisable(GL_CULL_FACE);

        glBindVertexArray(vertexArrayID); 

        glm::mat4 viewNoTrans = glm::mat4(glm::mat3(viewMatrix));
        glm::mat4 mvp = projectionMatrix * viewNoTrans;

        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(textureSamplerID, 0);

        glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
    }

    void cleanup() {
        glDeleteBuffers(1, &vertexBufferID);
        glDeleteBuffers(1, &colorBufferID);
        glDeleteBuffers(1, &uvBufferID);
        glDeleteBuffers(1, &indexBufferID);
        glDeleteTextures(1, &textureID);
        glDeleteVertexArrays(1, &vertexArrayID);
        glDeleteProgram(programID);
    }
};