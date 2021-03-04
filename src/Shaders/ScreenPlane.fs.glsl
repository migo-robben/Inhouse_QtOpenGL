#version 460 core

in vec2 coord;
out vec4 FragColor;

// texture samplers
uniform sampler2D map;

const float offset = 1.0 / 300.0;

const float kernelSharp[9] = float[](
    -1, -1, -1,
    -1,  9, -1,
    -1, -1, -1
);

const float kernelBlur[9] = float[](
    1.0 / 16, 2.0 / 16, 1.0 / 16,
    2.0 / 16, 4.0 / 16, 2.0 / 16,
    1.0 / 16, 2.0 / 16, 1.0 / 16
);

vec4 DefaultColor() {
    return texture(map, coord);
}

vec4 InversionColor() {
    return vec4(vec3(1.0 - texture(map, coord)), 1.0);
}

vec4 Grayscale() {
    vec4 tempFragColor = texture(map, coord);
    float average = 0.2126 * tempFragColor.r + 0.7152 * tempFragColor.g + 0.0722 * tempFragColor.b;
    tempFragColor = vec4(average, average, average, 1.0);
    return tempFragColor;
}

vec4 KernelEffects(float[9] kernel) {
    vec2 offsets[9] = vec2[](
    vec2(-offset,  offset), // 左上
    vec2( 0.0f,    offset), // 正上
    vec2( offset,  offset), // 右上
    vec2(-offset,  0.0f),   // 左
    vec2( 0.0f,    0.0f),   // 中
    vec2( offset,  0.0f),   // 右
    vec2(-offset, -offset), // 左下
    vec2( 0.0f,   -offset), // 正下
    vec2( offset, -offset)  // 右下
    );

    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(map, coord.st + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++)
    col += sampleTex[i] * kernel[i];

    return vec4(col, 1.0);
}

void main() {
    FragColor = DefaultColor();
}
