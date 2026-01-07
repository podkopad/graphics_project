#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in mat4 instanceMatrix; // New: 4 slots for mat4

out vec2 UV;
out vec3 Normal;

uniform mat4 view;
uniform mat4 projection;

void main() {
    // Multiply by the specific matrix for THIS tree instance
    gl_Position = projection * view * instanceMatrix * vec4(vertexPosition, 1.0);
    UV = vertexUV;
    Normal = mat3(instanceMatrix) * vertexNormal;
}