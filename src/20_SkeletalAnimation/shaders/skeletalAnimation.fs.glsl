#version 460 core

in vec2 coord;
out vec4 FragColor;

// texture samplers
uniform sampler2D colorMap;

void main() {
//    FragColor = texture(colorMap, coord);
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
