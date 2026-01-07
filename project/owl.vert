#version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;
layout(location = 3) in vec4 jointIndices;
layout(location = 4) in vec4 jointWeights;

uniform mat4 MVP;
uniform mat4 jointMatrices[50];

out vec2 UV;
out vec3 Normal;

void main() {
    mat4 skinMat = 
        jointWeights.x * jointMatrices[int(jointIndices.x)] +
        jointWeights.y * jointMatrices[int(jointIndices.y)] +
        jointWeights.z * jointMatrices[int(jointIndices.z)] +
        jointWeights.w * jointMatrices[int(jointIndices.w)];
    
    vec4 skinnedPos = skinMat * vec4(vertexPosition, 1.0);
    gl_Position = MVP * skinnedPos;
    
    UV = vertexUV;
    Normal = mat3(skinMat) * vertexNormal;
}