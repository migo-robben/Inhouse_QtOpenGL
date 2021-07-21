#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 mvp_matrix;

out vec3 Color;

void main()
{
    gl_Position = mvp_matrix * vec4(aPos, 1.0f);
    Color = aColor;
}