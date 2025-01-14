#version 330 core
layout(location = 0) in vec3 aPos;        // Vertex position
layout(location = 1) in vec3 aNormal;     // Vertex normal
layout(location = 2) in vec2 aTexCoords;  // Vertex texture coordinates

out vec2 TexCoords;   // Pass texture coordinates
out vec3 Normal;      // Pass normal
out vec3 FragPos;     // Pass fragment position

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float time;
uniform bool isShark;
uniform int meshID;

void main()
{
    vec3 modified_position = aPos;

    if (isShark) {
        float swayMultiplier = 1.5;
        float bodySway = sin(time * 2.5) * 0.7 + cos(time * 1.5) * 0.3;
        float influence = smoothstep(0.0, 1.0, abs(aPos.x) / 5.0);
        modified_position.z += swayMultiplier * bodySway * influence * sign(aPos.x);

        float verticalSway = cos(time * 1.5 + aPos.x * 0.2) * 0.15;
        modified_position.y += swayMultiplier * verticalSway * influence;

        if (meshID == 1) {
            float eyeSway = sin(time * 3.0) * 0.08;
            modified_position.y += swayMultiplier * eyeSway;
            modified_position.x += cos(time * 2.0) * 0.02;
        }
        else if (meshID == 2) {
            float teethSway = -sin(time * 3.5) * 0.06 + cos(time * 4.0) * 0.02;
            modified_position.y += swayMultiplier * teethSway;
        }
    }

    TexCoords = aTexCoords;
    Normal = mat3(transpose(inverse(model))) * aNormal; // Transform normal to world space
    FragPos = vec3(model * vec4(modified_position, 1.0)); // Apply modified position
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
