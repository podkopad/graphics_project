#version 330 core
in vec3 WorldNormal;
in float Visibility;

out vec4 FragColor;

void main() {
    // 1. Simple Directional Light (The "Sun")
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(WorldNormal), lightDir), 0.0);
    
    // 2. Base Colors
    vec3 terrainColor = vec3(0.12, 0.15, 0.10);
    vec3 ambient = terrainColor * 0.2;        
    vec3 finalTerrain = terrainColor * diff + ambient;

    // 3. Fog Color (Match this to your glClearColor in main.cpp)
    vec3 skyColor = vec3(0.2, 0.2, 0.25); 

    // 4. Mix based on visibility
    FragColor = vec4(mix(skyColor, finalTerrain, Visibility), 1.0);
}