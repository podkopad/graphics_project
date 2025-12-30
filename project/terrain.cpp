#include <stb/stb_perlin.h>  
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <render/shader.h>

struct Terrain {
    GLuint VAO, VBO, EBO;
    int gridSize;
    GLuint programID; 
    GLuint mvpMatrixID; 
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    float getHeight(float x, float z) {
        float scale = 0.05f;      // smaller = bigger hills
        float amplitude = 10.0f;  // height variation
        
        float noise = stb_perlin_noise3(x * scale, 0, z * scale, 0, 0, 0);
        return noise * amplitude;
    }
    
    void generateMesh() {
        // Generate vertices
        for (int z = 0; z < gridSize; z++) {
            for (int x = 0; x < gridSize; x++) {
                float xPos = (float)x;
                float zPos = (float)z;
                float yPos = getHeight(xPos, zPos);
                
                // Position
                vertices.push_back(xPos);
                vertices.push_back(yPos);
                vertices.push_back(zPos);
                
                // Normal (placeholder)
                float hL = getHeight(xPos - 1, zPos);
float hR = getHeight(xPos + 1, zPos);
float hD = getHeight(xPos, zPos - 1);
float hU = getHeight(xPos, zPos + 1);

// The "Finite Difference" method for normals
glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f, hD - hU));

vertices.push_back(normal.x);
vertices.push_back(normal.y);
vertices.push_back(normal.z);
                
                // UV coords
                vertices.push_back((float)x / gridSize);
                vertices.push_back((float)z / gridSize);
            }
        }
        
        // Generate indices
        for (int z = 0; z < gridSize - 1; z++) {
            for (int x = 0; x < gridSize - 1; x++) {
                int topLeft = z * gridSize + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * gridSize + x;
                int bottomRight = bottomLeft + 1;
                
                // First triangle
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);
                
                // Second triangle
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), 
                     vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                     indices.data(), GL_STATIC_DRAW);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                             (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // UV attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                             (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
    }
    
    void init(int size = 64) {
        gridSize = size;
        generateMesh();
        programID = LoadShadersFromFile(
            "C:/Users/User/Desktop/graphics/final proj/project/terr.vert",
            "C:/Users/User/Desktop/graphics/final proj/project/terr.frag"
        );
        mvpMatrixID = glGetUniformLocation(programID, "MVP");
    }
    
    void render(glm::mat4 mvp) {
            glUseProgram(programID);  // <- YOU WERE MISSING THIS!
    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteProgram(programID); 
    }
};