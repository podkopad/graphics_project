#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 UV;

out vec4 FragColor;

void main() {
    // Green terrain with lighting
    vec3 color = vec3(0.2, 0.6, 0.3);
    
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = diff * color;
    
    vec3 ambient = 0.3 * color;
    
    FragColor = vec4(ambient + diffuse, 1.0);
}