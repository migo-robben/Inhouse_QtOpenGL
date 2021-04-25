#version 460 core
#define SHBAND2 9

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in vec3 ObjectSHCoefficient[SHBAND2];

uniform vec3 LightSHCoefficient[SHBAND2];

out vec3 Color;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0f);

    vec3 accColor = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < SHBAND2; ++i) {
        accColor += LightSHCoefficient[i] * ObjectSHCoefficient[i];
    }
    Color = accColor;
}
