#version 460 core

in vec2 coord;
out vec4 FragColor;

// texture samplers
uniform sampler2D map;

void main() {
    vec3 selelectColor = vec3(0.1, 1.0, 0.6);
    FragColor = vec4(mix(texture(map, coord).rgb, selelectColor, 0.2), 1.0);
}

