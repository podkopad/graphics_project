#version 330 core

out vec3 finalColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    vec3 skyColor = texture(skybox, TexCoords).rgb;
    
    // Make it darker (multiply by a value < 1.0)
    skyColor = pow(skyColor, vec3(1.5));
    vec3 eveningTint = vec3(0.85, 0.8, 1.0); 
    skyColor *= eveningTint;
    vec3 gloomyColor = vec3(0.2, 0.22, 0.3);
    skyColor = mix(skyColor, gloomyColor, 0.4);
    float luminance = dot(skyColor, vec3(0.2126, 0.7152, 0.0722));
    skyColor = mix(vec3(luminance), skyColor, 0.6);
    vec3 dir = normalize(TexCoords);
    float skyDarken = smoothstep(0.0, 0.6, dir.y);
skyColor *= mix(1.0, 0.5, skyDarken);
vec3 horizonColor = vec3(0.07, 0.10, 0.16); // dirty orange
float horizonFactor = smoothstep(-0.1, 0.2, dir.y);
skyColor = mix(horizonColor, skyColor, horizonFactor*0.9);

skyColor.r *=0.8;
//skyColor = 1.0 - pow(1.0 - skyColor, vec3(2.0));
    
    // Add cooler tone (boost blue, reduce red/green slightly)
    //skyColor.r *= 1.2;  // Less red
    //skyColor.g *= 0.8;   // Less green
    //skyColor.b *= 1.2;   // More blue
    
    // Simple moon: create a bright spot in the sky
   // vec3 moonDir = normalize(vec3(0.5, 0.8, 0.3)); // Moon direction
    //float moonDot = dot(normalize(TexCoords), moonDir);
    //float moonGlow = smoothstep(0.996, 0.999, moonDot); // Sharp moon
    //float moonHalo = smoothstep(0.99, 0.996, moonDot) * 0.3; // Soft glow around moon
    
    //vec3 moonColor = vec3(1.0, 1.0, 0.95) * (moonGlow + moonHalo);
    
    finalColor = skyColor;
}