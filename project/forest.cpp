#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <random>

struct TreeInstance {
    glm::mat4 modelMatrix;
};

class Forest {
public:
    GLuint instanceVBO = 0;
    std::vector<TreeInstance> trees;

    // Pathway Settings
    float pathWidth = 15.0f;
    float pathFrequency = 0.02f;
    float pathAmplitude = 35.0f;

    // 1. Logic to clear a winding path
    bool isOnPath(float x, float z) {
        float pathX = sin(z * pathFrequency) * pathAmplitude;
        return abs(x - pathX) < pathWidth;
    }

    // 2. Function to create trees based on terrain
    void regenerateForVisibleChunks(class TerrainManager& terrain) {
        trees.clear();
        std::vector<glm::mat4> matrices;

        for (auto& chunk : terrain.chunks) {
            if (!chunk.isReady) continue;

            // Seed based on chunk offset so trees stay in the same place
            std::mt19937 rng(static_cast<unsigned int>(chunk.offset.x * 73856093) ^ 
                             static_cast<unsigned int>(chunk.offset.y * 19349663));
            std::uniform_real_distribution<float> dist(0.0f, 64.0f);
            std::uniform_real_distribution<float> scaleDist(1.5f, 4.0f);

            for (int i = 0; i < 1; i++) { // 60 trees per chunk
                float x = chunk.offset.x + dist(rng);
                float z = chunk.offset.y + dist(rng);

                if (isOnPath(x, z)) continue;

                // Match terrain height (using the terrain's noise math)
                float y = chunk.getHeight(x, z) - 10.0f; 

                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
                model = glm::scale(model, glm::vec3(scaleDist(rng)));
                matrices.push_back(model);
                trees.push_back({model});
            }
        }

        // Upload data to GPU
        if (instanceVBO == 0) glGenBuffers(1, &instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, matrices.size() * sizeof(glm::mat4), matrices.data(), GL_DYNAMIC_DRAW);
    }

    // 3. Link the instance data to a Tree's VAO
    void setupInstancedRendering(GLuint treeVAO) {
        glBindVertexArray(treeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

        for (int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(3 + i); 
            glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
            glVertexAttribDivisor(3 + i, 1); // Tell OpenGL this is per-instance (per tree)
        }
        glBindVertexArray(0);
    }

    void cleanup() {
        if (instanceVBO != 0) glDeleteBuffers(1, &instanceVBO);
    }
};