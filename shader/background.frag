#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 topColor;         // Top background color
uniform vec3 bottomColor;      // Bottom background color
uniform float time;            // Time(for dynamic)
uniform vec3 sunPosition;      // Sunshine simulation

void main() 
{
    // Water simulation
    vec3 color = mix(bottomColor, topColor, TexCoords.y);

    // Light shaft for background
    float lightShaft = sin(TexCoords.y * 5.0 + time * 0.2) * 0.05; // Frequency & Intensity
    lightShaft += cos(TexCoords.x * 3.0 + time * 0.1) * 0.03;      // Decrease dynamic changing
    lightShaft = smoothstep(0.0, 0.1, lightShaft);                 // Smoothstep
    lightShaft = clamp(lightShaft, 0.0, 0.1);                

    // Mix light shaft
    vec3 finalColor = color + lightShaft * vec3(0.5 * sunPosition.x, 0.7, 0.5 * sunPosition.z);

    // Combine final color
    FragColor = vec4(finalColor, 1.0);
}
