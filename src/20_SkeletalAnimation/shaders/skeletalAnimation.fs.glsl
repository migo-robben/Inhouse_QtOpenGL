#version 460 core

in vec2 coord;
in vec3 Normal;
in vec3 WorldPos;

out vec4 FragColor;

// texture samplers
uniform sampler2D colorMap;

void main() {
//    FragColor = texture(colorMap, coord);
    FragColor = vec4(Normal.x, Normal.y, Normal.z, 1.0);
}
