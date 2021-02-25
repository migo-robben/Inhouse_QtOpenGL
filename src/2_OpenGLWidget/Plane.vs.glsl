#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_color;

uniform mat4 mvp_matrix;
out vec3 color;

void main() {
    gl_Position = mvp_matrix * vec4(a_position, 1.0f);
    color = a_color;
}
