#version 460 core

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
//uniform sampler2D texNoise;
//uniform sampler2D gDepth;

uniform vec3 samples[128];

uniform mat4 projection;

in vec2 Coords;
out float FragColor;

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 64;
float radius = 15.5;
float bias = 0.025;

// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(540.0/4.0, 540.0/4.0);

int texSamples = 4;
vec4 textureMultisample(sampler2DMS sampler, ivec2 coord)
{
    vec4 color = vec4(0.0);

    for (int i = 0; i < texSamples; i++)
    color += texelFetch(sampler, coord, i);

    color /= float(texSamples);

    return color;
}

void main() {
    vec2 uv = vec2(Coords.x, Coords.y);
    ivec2 texSize = textureSize(gNormal);
    ivec2 texCoord = ivec2(uv * texSize);
    vec4 color = textureMultisample(gNormal, texCoord);

    FragColor = color.r;
}
