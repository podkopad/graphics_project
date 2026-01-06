// terrain.cpp
#include <stb/stb_perlin.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <render/shader.h>
#include <vector>
#include <map>
#include <future>

struct MeshData {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

struct TerrainChunk {
    GLuint VAO = 0, VBO = 0, EBO = 0;
    glm::vec2 offset;
    glm::vec2 targetOffset;
    int indexCount = 0;
    bool isReady = false;
    bool isCalculating = false;

    float getHeight(float x, float z) {
    float total = 0.0f;
    float persistence = 0.5f;
    float amplitude = 15.0f; 
    float freq = 0.015f;      
    
    // 4 Octaves for higher detail
    for (int i = 0; i < 3; i++) {
        total += stb_perlin_noise3(x * freq, 0, z * freq, 0, 0, 0) * amplitude;
        
        amplitude *= persistence; // Each octave is half as strong
        freq *= 2.0f;             // Each octave is twice as detailed
    }
    
    return total;
}

    MeshData generateMeshData(glm::vec2 worldOffset, int size) {
        MeshData data;
        int verticesAcross = size + 1;
        for (int z = 0; z <= size; z++) {
            for (int x = 0; x <= size; x++) {
                float xPos = (float)x + worldOffset.x;
                float zPos = (float)z + worldOffset.y;
                float yPos = getHeight(xPos, zPos);

                data.vertices.push_back((float)x);
                data.vertices.push_back(yPos);
                data.vertices.push_back((float)z);

                // Simple Normals
                float delta = 0.1f; // Smaller delta for sharper normals
float hL = getHeight(xPos - delta, zPos);
float hR = getHeight(xPos + delta, zPos);
float hD = getHeight(xPos, zPos - delta);
float hU = getHeight(xPos, zPos + delta);
// This cross product is the standard way to get a surface normal from a heightmap
glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * delta, hD - hU));

                data.vertices.push_back(normal.x);
                data.vertices.push_back(normal.y);
                data.vertices.push_back(normal.z);
                data.vertices.push_back((float)x / size);
                data.vertices.push_back((float)z / size);
            }
        }
        for (int z = 0; z < size; z++) {
            for (int x = 0; x < size; x++) {
                int topLeft = z * verticesAcross + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * verticesAcross + x;
                int bottomRight = bottomLeft + 1;
                data.indices.push_back(topLeft); data.indices.push_back(bottomLeft); data.indices.push_back(topRight);
                data.indices.push_back(topRight); data.indices.push_back(bottomLeft); data.indices.push_back(bottomRight);
            }
        }
        return data;
    }

    void uploadToGPU(const MeshData& data) {
        if (VAO == 0) {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
        }
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(float), data.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(unsigned int), data.indices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        indexCount = data.indices.size();
        isReady = true;
        isCalculating = false;
    }

    void cleanup() {
        if (VAO != 0) { glDeleteVertexArrays(1, &VAO); glDeleteBuffers(1, &VBO); glDeleteBuffers(1, &EBO); }
    }
};

class TerrainManager {
public:
    std::vector<TerrainChunk> chunks;
    std::map<int, std::future<MeshData>> futures;
    GLuint programID;
    GLuint mvpLoc, modelLoc, camPosLoc;
    int chunkSize;
    int gridWidth;
    glm::vec2 lastCenter;

    void init(int size, int width) {
        chunkSize = size;
        gridWidth = width;
        chunks.resize(width * width);

        for(auto& c : chunks) {
        c.offset = glm::vec2(-99999.0f); 
        c.isReady = false;
    }
        programID = LoadShadersFromFile(
            "C:/Users/User/Desktop/graphics/final proj/project/terr.vert",
            "C:/Users/User/Desktop/graphics/final proj/project/terr.frag"
        );
        mvpLoc = glGetUniformLocation(programID, "MVP");
        modelLoc = glGetUniformLocation(programID, "Model");
        camPosLoc = glGetUniformLocation(programID, "cameraPos");

        // Initialize ALL chunks in the grid immediately (synchronously for startup)
        lastCenter = glm::vec2(0, 0);
        int half = gridWidth / 2;
        
        for (int z = 0; z < gridWidth; z++) {
            for (int x = 0; x < gridWidth; x++) {
                int idx = z * gridWidth + x;
                glm::vec2 offset(
                    (x - half) * chunkSize,
                    (z - half) * chunkSize
                );
                chunks[idx].offset = offset;
                chunks[idx].targetOffset = offset;
                
                MeshData data = chunks[idx].generateMeshData(offset, chunkSize);
                chunks[idx].uploadToGPU(data);
            }
        }
    }

void update(glm::vec3 cameraPos) {
    glm::vec2 currentCenter(
        round(cameraPos.x / (float)chunkSize),
        round(cameraPos.z / (float)chunkSize)
    );

    if (glm::distance(currentCenter, lastCenter) > 0.5f) {
        updateGrid(currentCenter);
        lastCenter = currentCenter;
    }

    // Handshake: Check if any background threads are done
    for (auto it = futures.begin(); it != futures.end(); ) {
        // We use wait_for(0) to check status without blocking the game
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            int idx = it->first;
            
            // 1. Get the generated mesh data
            MeshData data = it->second.get();
            
            // 2. NOW update the world offset to the new destination
            chunks[idx].offset = chunks[idx].targetOffset;
            
            // 3. Upload to GPU and mark as ready
            chunks[idx].uploadToGPU(data);
            
            // Remove from the "waiting" list
            it = futures.erase(it);
        } else {
            ++it;
        }
    }
}

        void updateGrid(glm::vec2 center) {
            int half = gridWidth / 2;
            
            // Create a list of all required offsets for the new grid
            std::vector<glm::vec2> requiredOffsets;
            for (int z = 0; z < gridWidth; z++) {
                for (int x = 0; x < gridWidth; x++) {
                    requiredOffsets.push_back(glm::vec2(
                        (center.x + x - half) * chunkSize,
                        (center.y + z - half) * chunkSize
                    ));
                }
            }

            for (const auto& targetPos : requiredOffsets) {
                // 1. Check if ANY chunk already has this offset
                bool alreadyExists = false;
                for (auto& c : chunks) {
                    if (c.offset == targetPos || (c.isCalculating && c.targetOffset == targetPos)) {
                        alreadyExists = true;
                        break;
                    }
                }

                // 2. If it doesn't exist, find the FURTHEST chunk to recycle
                if (!alreadyExists) {
                    float maxDist = -1.0f;
                    int bestIdx = -1;

                    for (int i = 0; i < chunks.size(); i++) {
                        if (chunks[i].isCalculating) continue; // Don't interrupt busy chunks
                        
                        float d = glm::distance(chunks[i].offset, center * (float)chunkSize);
                        if (d > maxDist) {
                            maxDist = d;
                            bestIdx = i;
                        }
                    }

                    if (bestIdx != -1) {
                        chunks[bestIdx].isCalculating = true;
                        chunks[bestIdx].targetOffset = targetPos;
                        
                        // Launch thread
                        futures[bestIdx] = std::async(std::launch::async, [this, bestIdx, targetPos]() {
                            return chunks[bestIdx].generateMeshData(targetPos, chunkSize);
                        });
                    }
                }
            }
        }

    void render(glm::mat4 view, glm::mat4 proj, glm::vec3 camPos) {
        glUseProgram(programID);
        glUniform3fv(camPosLoc, 1, &camPos[0]);

        for (auto& chunk : chunks) {
            if (!chunk.isReady) continue;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(chunk.offset.x, -10.0f, chunk.offset.y));
            glm::mat4 mvp = proj * view * model;
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
            glBindVertexArray(chunk.VAO);
            glDrawElements(GL_TRIANGLES, chunk.indexCount, GL_UNSIGNED_INT, 0);
        }
    }

    void cleanup() {
        for (auto& c : chunks) c.cleanup();
        glDeleteProgram(programID);
    }
};