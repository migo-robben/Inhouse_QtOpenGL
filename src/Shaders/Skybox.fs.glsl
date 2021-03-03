#version 460 core
#extension GL_NV_shadow_samplers_cube : enable

in vec3 coord;
out vec4 FragColor;

// texture samplers
uniform samplerCube cubeMap;

void main() {
    FragColor = textureCube(cubeMap, coord);
}
