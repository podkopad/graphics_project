#version 330 core

in vec3 fragColor;
in vec3 fragNormal;
in vec3 fragPos;

out vec4 FragColor;

uniform vec3 lightDir;   // Moon direction from main.cpp
uniform vec3 cameraPos;  // eye_center from main.cpp

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-lightDir); // Direction TO the light
    vec3 V = normalize(cameraPos - fragPos);
    vec3 H = normalize(L + V); // Halfway vector for highlights

    // 1. Ambient: base "dark" color so it's not pitch black
    vec3 ambient = 0.2 * fragColor;

    // 2. Diffuse: This creates the 3D curve/shadow
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * fragColor;

    // 3. Specular: The "shiny" hotspot that makes it look like a 3D ball
    float spec = pow(max(dot(N, H), 0.0), 32.0); // 32 is shininess
    vec3 specular = vec3(0.5) * spec; 

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}