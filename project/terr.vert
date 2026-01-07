#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out vec3 WorldNormal;
out float Visibility;

uniform mat4 MVP;
uniform mat4 Model;
uniform vec3 cameraPos;

void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
    vec4 worldPos = Model * vec4(aPos, 1.0);

    // Pass the normal to fragment shader, adjusted for rotation/scale
    WorldNormal = mat3(transpose(inverse(Model))) * aNormal;

    // Linear Fog calculation
    float distance = length(worldPos.xyz - cameraPos);
    float fogStart = 100.0;
    float fogEnd = 600.0;
    Visibility = (fogEnd - distance) / (fogEnd - fogStart);
    Visibility = clamp(Visibility, 0.0, 1.0);
}