#version 330 core
layout(location = 0) in vec3 vertexPosition;
out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 moonPos;
uniform float moonSize;

void main() {
    vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 camUp = vec3(view[0][1], view[1][1], view[2][1]);
    
    vec3 pos = moonPos + (camRight * vertexPosition.x * moonSize) + (camUp * vertexPosition.y * moonSize);
    gl_Position = projection * view * vec4(pos, 1.0);
    
    // Map -1.0 to 1.0 range to 0.0 to 1.0 for UVs
    TexCoords = vertexPosition.xy * 0.5 + 0.5;
}