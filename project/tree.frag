#version 330 core

in vec2 UV;
in vec3 Normal;

out vec4 color;

uniform sampler2D textureSampler;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(Normal), lightDir), 0.3);
    
    vec4 texColor = texture(textureSampler, UV);
    color = vec4(texColor.rgb * diff, texColor.a);
}