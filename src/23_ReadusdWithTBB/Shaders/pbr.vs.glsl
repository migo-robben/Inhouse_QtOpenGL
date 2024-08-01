#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aDisplayColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 coord;
out vec3 WorldPos;
out vec3 Normal;
out vec3 displayColor;


void main() {
    displayColor = aDisplayColor;
    coord = aCoord;
    WorldPos = vec3(model * vec4(aPos, 1.0));

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * aNormal);

    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}