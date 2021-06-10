#version 460 core

uniform sampler2D colorBuffer;

in vec2 Coords;
out vec4 FragColor;

void main() {
    vec3 color = texture(colorBuffer, Coords).xyz;
    FragColor = vec4(color, 1.0);
}
