#version 460 core

in vec3 displayColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(displayColor, 1.0f);
}
