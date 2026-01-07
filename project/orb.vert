#version 330 core

layout(location = 0) in vec3 vertexPosition;

// Per-instance data
layout(location = 1) in vec4 instanceMatrix0;
layout(location = 2) in vec4 instanceMatrix1;
layout(location = 3) in vec4 instanceMatrix2;
layout(location = 4) in vec4 instanceMatrix3;
layout(location = 5) in vec3 instanceColor;

uniform mat4 view;
uniform mat4 projection;

out vec3 fragColor;
out vec3 fragNormal;  // ADD THIS
out vec3 fragPos;     // ADD THIS

void main() {
    mat4 instanceModel = mat4(instanceMatrix0, instanceMatrix1, instanceMatrix2, instanceMatrix3);
    vec4 worldPos = instanceModel * vec4(vertexPosition, 1.0);
    gl_Position = projection * view * worldPos;
    
    fragColor = instanceColor;
    fragNormal = mat3(instanceModel) * vertexPosition;  // For sphere, position IS the normal!
    fragPos = worldPos.xyz;
}