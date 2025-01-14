#version 330 core
out vec4 FragColor;

in vec2 TexCoords;   // Texture coordinates passed in
in vec3 Normal;      // Normal vector passed in
in vec3 FragPos;     // Fragment position in world space

uniform sampler2D texture_diffuse; // Texture sampler
uniform vec3 ambientLight;         // Ambient light color
uniform vec3 lightColor;           // Light source color
uniform vec3 lightDir;             // Light source direction (directional light)
uniform vec3 viewPos;              // Camera position

// Fog-related uniforms
uniform float fogDensity;          // Fog density control
uniform vec3 fogColor;             // Fog color

void main()
{
    // Sample color from the texture
    vec4 texColor = texture(texture_diffuse, TexCoords);

    // Calculate ambient lighting
    vec3 ambient = ambientLight * texColor.rgb;

    // Normalize the normal vector
    vec3 norm = normalize(Normal);
    
    // Calculate diffuse lighting
    vec3 dirLight = normalize(-lightDir);
    float diff = max(dot(norm, dirLight), 0.0);
    vec3 diffuse = diff * lightColor * texColor.rgb;

    // Calculate specular highlights
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-dirLight, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = lightColor * spec * 0.6;

    // Scattered light (additional scattering effect to slightly simulate underwater light scattering)
    float scatterFactor = pow(dot(norm, dirLight), 2.0);
    vec3 scatteredLight = lightColor * scatterFactor;

    // Apply distance attenuation to the lighting
    float distance = length(viewPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.1 * distance);
    diffuse *= attenuation;
    specular *= attenuation;
    scatteredLight *= attenuation;

    // Combine final base color (without fog)
    vec3 result = ambient + diffuse + specular + scatteredLight;

    // Fog processing
    // Calculate fog factor based on fragment distance and fogDensity
    float fogFactor = exp(-pow(distance * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // Interpolate between object color and fog color
    vec3 finalColor = mix(fogColor, result, fogFactor);

    // Output the final result
    FragColor = vec4(finalColor, texColor.a);
}
