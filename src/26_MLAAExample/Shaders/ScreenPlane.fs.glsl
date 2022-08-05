#version 460 core

in vec2 coord;
out vec4 FragColor;

// texture samplers
uniform sampler2D map;

void main() {
    FragColor = texture(map, coord);
}
