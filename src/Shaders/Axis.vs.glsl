#version 460 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec3 position;

void main() {
    vec4 tempP = projection * view * model * vec4(position, 1);
    gl_Position = vec4(tempP.xy, 0.0, 1.0);
}