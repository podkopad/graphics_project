#version 330 core

in vec3 fragColor;
in vec3 fragNormal;
in vec3 fragPos;

out vec4 FragColor;

uniform vec3 moonDir = vec3(0.5, 1.0, -0.5);

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 lightDirection = normalize(-moonDir);
    
    // Diffuse lighting
    float diff = max(dot(norm, lightDirection), 0.0);
    
    // Strong ambient so they glow
    vec3 ambient = fragColor * 0.6;
    vec3 diffuse = fragColor * diff * 0.4;
    
    //extra glow
    vec3 viewDir = normalize(-fragPos);
    float rim = 1.0 - max(dot(norm, viewDir), 0.0);
    rim = pow(rim, 3.0);
    
    vec3 result = ambient + diffuse + rim * fragColor * 0.5;
    FragColor = vec4(result, 1.0);
}