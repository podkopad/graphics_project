#version 330 core

in vec2 UV;
in vec3 Normal;

out vec4 color;

uniform sampler2D textureSampler;
uniform vec3 lightDir;      // Direction from the moon
uniform vec3 lightColor;    // Pale blue/white for the moon
uniform vec3 ambientColor;  // Dark blue/black for night shadows

void main() {
    vec3 n = normalize(Normal);
    vec3 l = normalize(lightDir);
    
    // Diffuse lighting
    float diff = max(dot(n, l), 0.0);
    
    // Final color calculation
    vec4 texColor = texture(textureSampler, UV);
    
    // Apply moon light + ambient night light
    vec3 finalRGB = texColor.rgb * (diff * lightColor + ambientColor);
    
    color = vec4(finalRGB, texColor.a);
}