#version 330 core
out vec4 color;
in vec2 TexCoords; // Pass this from vertex shader

void main() {
    // Calculate distance from center (0.5, 0.5)
    float distance = length(TexCoords - vec2(0.5));
    
    // Discard pixels outside the circle radius (0.5)
    if (distance > 0.5) {
        discard;
    }

    // Optional: Add a soft glow/feathered edge
    float glow = 1.0 - smoothstep(0.4, 0.5, distance);
    color = vec4(1.0, 1.0, 0.9, glow); 
}