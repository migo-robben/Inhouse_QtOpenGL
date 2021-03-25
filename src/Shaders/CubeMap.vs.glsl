#version 460 core

layout (location = 0) in vec3 aPos;

out vec3 WorldPos;

uniform mat4 view;
uniform mat4 projection;

void main() {
    WorldPos = aPos;
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}