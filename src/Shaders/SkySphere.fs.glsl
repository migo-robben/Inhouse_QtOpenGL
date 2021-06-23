#version 460 core


in vec2 coord;
in vec3 WorldPos;
in vec3 Normal;

out vec4 outColor;
uniform vec4 color;

void main() {
    outColor = vec4(Normal ,1.0);
}