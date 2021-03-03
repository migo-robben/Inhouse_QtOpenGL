#version 460 core

in vec2 coord;
out vec4 FragColor;

// texture samplers
uniform sampler2D map;

void main()
{
    FragColor = vec4(mix(texture(map, coord), vec4(1.0, 0.5, 0.2, 1.0), 0.0));
}
