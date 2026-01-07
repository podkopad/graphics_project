// glowing_orbs.cpp
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <set>
#include <utility>
#include <random>
#include <ctime>
#include <iostream>

struct GlowingOrb {
    glm::vec3 position;
    glm::vec3 color;
    float phase;
    float radius;
    glm::vec3 center;
};

class OrbSystem {
public:
    std::vector<GlowingOrb> orbs;
    std::set<std::pair<int, int>> spawnedChunks;
    GLuint sphereVAO = 0;
    GLuint sphereVBO = 0;
    GLuint sphereEBO = 0;
    GLuint instanceVBO = 0;
    int sphereIndexCount = 0;
    int orbsPerChunk = 5;
    
    void init() {
        srand(time(NULL));
        createSphereMesh();
        std::cout << "Orb system initialized!" << std::endl;
    }
    
    void regenerateForVisibleChunks(class TerrainManager& terrain) {
        bool addedNew = false;
        for (auto& chunk : terrain.chunks) {
            if (chunk.isReady) {
                // Convert chunk offset to a key (e.g., 128.0f -> 1)
                int ix = (int)std::round(chunk.offset.x / terrain.chunkSize);
                int iy = (int)std::round(chunk.offset.y / terrain.chunkSize);
                std::pair<int, int> chunkCoord = {ix, iy};

                // Only generate if we haven't seen this chunk before
                if (spawnedChunks.find(chunkCoord) == spawnedChunks.end()) {
                    generateOrbsForChunk(chunk.offset, terrain.chunkSize);
                    spawnedChunks.insert(chunkCoord);
                    addedNew = true;
                }
            }
        }
    }
    
    void generateOrbsForChunk(glm::vec2 chunkOffset, int chunkSize) {
        // Use the chunk offset as a seed so the same chunk ALWAYS gets the same orbs
        std::mt19937 rng(static_cast<unsigned int>(chunkOffset.x * 73856093 + chunkOffset.y * 19349663));
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < orbsPerChunk; i++) {
            GlowingOrb orb;
            // Position within the chunk boundaries
            float x = chunkOffset.x + dist(rng) * chunkSize;
            float z = chunkOffset.y + dist(rng) * chunkSize;
            float y = 5.0f + dist(rng) * 15.0f; // Floating height
            
            orb.center = glm::vec3(x, y, z);
            
            // Consistent color based on the seed
            float colorChoice = dist(rng);
            if (colorChoice < 0.33f) orb.color = glm::vec3(0.2f, 0.6f, 1.0f);
            else if (colorChoice < 0.66f) orb.color = glm::vec3(1.0f, 0.2f, 0.8f);
            else orb.color = glm::vec3(0.6f, 0.2f, 1.0f);
            
            orb.phase = dist(rng) * 6.28f;
            orb.radius = 2.0f + dist(rng) * 3.0f;
            orbs.push_back(orb);
        }
    }
    
    void createSphereMesh() {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        
        int rings = 10;
        int sectors = 10;
        float radius = 0.5f;
        
        for (int r = 0; r <= rings; r++) {
            float phi = M_PI * r / rings;
            for (int s = 0; s <= sectors; s++) {
                float theta = 2.0f * M_PI * s / sectors;
                
                float x = radius * sin(phi) * cos(theta);
                float y = radius * cos(phi);
                float z = radius * sin(phi) * sin(theta);
                
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
            }
        }
        
        for (int r = 0; r < rings; r++) {
            for (int s = 0; s < sectors; s++) {
                int first = r * (sectors + 1) + s;
                int second = first + sectors + 1;
                
                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);
                
                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }
        
        sphereIndexCount = indices.size();
        
        glGenVertexArrays(1, &sphereVAO);
        glGenBuffers(1, &sphereVBO);
        glGenBuffers(1, &sphereEBO);
        
        glBindVertexArray(sphereVAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindVertexArray(0);
    }
    
    void update(float time) {
        for (auto& orb : orbs) {
            // SLOWED DOWN: 0.1 = gentle floating
            float animTime = time * 0.3f + orb.phase;
            
            float offsetX = cos(animTime) * orb.radius;
            float offsetZ = sin(animTime) * orb.radius;
            float offsetY = sin(animTime * 2.0f) * 1.5f;
            
            orb.position = orb.center + glm::vec3(offsetX, offsetY, offsetZ);
        }
    }
    
    void render(GLuint shaderProgram, glm::mat4 view, glm::mat4 projection) {
        if (orbs.empty()) return;
        
        glUseProgram(shaderProgram);
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        
        // Build instance data
        std::vector<float> instanceData;
        for (const auto& orb : orbs) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, orb.position);
            model = glm::scale(model, glm::vec3(2.5f));
            
            // Matrix in column-major order
            for (int col = 0; col < 4; col++) {
                for (int row = 0; row < 4; row++) {
                    instanceData.push_back(model[col][row]);
                }
            }
            
            // Color
            instanceData.push_back(orb.color.r);
            instanceData.push_back(orb.color.g);
            instanceData.push_back(orb.color.b);
        }
        
        if (instanceVBO == 0) {
            glGenBuffers(1, &instanceVBO);
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), 
                     instanceData.data(), GL_DYNAMIC_DRAW);
        
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        
        size_t stride = 19 * sizeof(float);
        
        // Matrix (locations 1-4)
        for (int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(1 + i);
            glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, stride, 
                                 (void*)(i * 4 * sizeof(float)));
            glVertexAttribDivisor(1 + i, 1);
        }
        
        // Color (location 5)
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, stride, 
                             (void*)(16 * sizeof(float)));
        glVertexAttribDivisor(5, 1);
        
        glDrawElementsInstanced(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0, orbs.size());
        
        glBindVertexArray(0);
    }
    
    void cleanup() {
        if (sphereVAO) glDeleteVertexArrays(1, &sphereVAO);
        if (sphereVBO) glDeleteBuffers(1, &sphereVBO);
        if (sphereEBO) glDeleteBuffers(1, &sphereEBO);
        if (instanceVBO) glDeleteBuffers(1, &instanceVBO);
    }
};