#version 460 core

in vec3 Color;
out vec4 FragColor;

void main() {
    FragColor = vec4(pow(Color, vec3(1.0 / 1.5)), 1.0);
}
