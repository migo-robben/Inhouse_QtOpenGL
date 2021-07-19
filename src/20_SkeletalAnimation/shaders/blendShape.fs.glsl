#version 460 core

in vec2 coord;

out vec4 FragColor;

uniform sampler2D colorMap;

void main() {
        FragColor = texture(colorMap, coord);
}
