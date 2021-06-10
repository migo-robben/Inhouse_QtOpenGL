#version 460

layout(location = 0) out vec4 FragColor;

layout (binding = 0) uniform sampler2D gReflectionSampler;
layout (binding = 1) uniform sampler2D gColorSampler;
//layout (binding = 2) uniform sampler2D ssaoColorBufferBlur;

uniform int TotalFrames;
uniform bool isMoving;
//uniform int HBAOOn;

in vec2 coord;

void main() {
    vec3 color = vec3(texture(gColorSampler, coord).rgb);
    vec3 reflection = (texture(gReflectionSampler, coord).rgb);

    if (!isMoving && TotalFrames < 252) {
        reflection /= TotalFrames;
    }

    FragColor = vec4(vec3(reflection + color * 1.0), 1.0);
}
