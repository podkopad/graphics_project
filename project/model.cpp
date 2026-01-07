#include <glad/gl.h>
#include <glm/glm.hpp>
#include <stb/stb_image.h>            
#include "../external/tiny_obj_loader.h" 
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>

struct Model {
    GLuint VAO, VBO;
    GLuint programID;
    GLuint mvpMatrixID;
    GLuint textureID;
    int vertexCount;

    void init(const char* objPath, const char* texturePath, GLuint shaderProgram) {
        programID = shaderProgram;
        mvpMatrixID = glGetUniformLocation(programID, "MVP");

        // Load OBJ file
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath);
        
        if (!warn.empty()) {
            std::cout << "WARN: " << warn << std::endl;
        }
        if (!err.empty()) {
            std::cerr << "ERR: " << err << std::endl;
        }
        if (!ret) {
            std::cerr << "Failed to load " << objPath << std::endl;
            return;
        }

        std::cout << "Loaded " << objPath << " successfully!" << std::endl;
        std::cout << "Vertices: " << attrib.vertices.size() / 3 << std::endl;

        // Flatten vertices for OpenGL
        std::vector<float> vertices;
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                // Position
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);
                
                // Texture coords (if available)
                if (index.texcoord_index >= 0) {
    vertices.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
    vertices.push_back(1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }

                // Normal (if available)
                if (index.normal_index >= 0) {
                    vertices.push_back(attrib.normals[3 * index.normal_index + 0]);
                    vertices.push_back(attrib.normals[3 * index.normal_index + 1]);
                    vertices.push_back(attrib.normals[3 * index.normal_index + 2]);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(1.0f);
                    vertices.push_back(0.0f);
                }
            }
        }

        vertexCount = vertices.size() / 8; // 3 pos + 2 uv + 3 normal = 8 floats per vertex

        // Setup VAO/VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

        // UV attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

        // Normal attribute
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

        glBindVertexArray(0);

        // Load texture
        loadTexture(texturePath);
    }

    void loadTexture(const char* path) {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        int width, height, nrChannels;
        unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ")" << std::endl;
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }
        stbi_image_free(data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void render(glm::mat4 mvp) {
        glUseProgram(programID);
        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(programID, "textureSampler"), 0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }
};