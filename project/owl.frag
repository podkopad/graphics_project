#version 330 core

in vec2 UV;
in vec3 Normal;

out vec4 color;

uniform vec3 lightPosition;
uniform vec3 lightIntensity;

void main() {
    vec3 lightDir = normalize(lightPosition);
    float diff = max(dot(normalize(Normal), lightDir), 0.3);
    
    vec3 owlColor = vec3(0.6, 0.5, 0.4); // Brown owl
    color = vec4(owlColor * diff, 1.0);
}