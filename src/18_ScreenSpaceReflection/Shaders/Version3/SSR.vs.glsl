#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aCoord;

out vec2 Coords;

void main() {
    Coords = aCoord;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
