#version 460 core

in vec2 coord;
in vec3 Normal;
in vec3 WorldPos;
in vec3 DebugColor;

out vec4 FragColor;

// texture samplers
uniform sampler2D colorMap;

void main() {
//    FragColor = texture(colorMap, coord);
    FragColor = vec4(Normal.x, Normal.y, Normal.z, 1.0);
//    FragColor = vec4(coord.x, coord.y, 0.0, 1.0);
//    vec4 color = texture(blendShapeMap1, coord);
//    FragColor = color;
//    FragColor = vec4(DebugColor, 1.0);
}
