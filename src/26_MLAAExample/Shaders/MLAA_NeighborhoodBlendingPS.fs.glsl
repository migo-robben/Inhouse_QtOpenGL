#version 460

in vec2 coord;
out vec4 FragColor;

uniform vec2 resolution;

// texture samplers
uniform sampler2D blendTex;
uniform sampler2D colorTex;
uniform sampler2D alphaTex;

vec4 mad(vec4 m, vec4 a, vec4 b) {
    return m * a + b;
}

vec4 NeighborhoodBlendingPS(vec2 texcoord) {
    vec4 topLeft = texture(blendTex, texcoord);
    float right = texture(alphaTex, texcoord + vec2(1, 0) * resolution).r;
    float bottom = texture(blendTex, texcoord - vec2(0, 1) * resolution).g;

    // 上，下，左，右
    vec4 a = vec4(topLeft.r, bottom, topLeft.b, right);

    vec4 w = a;
    float sum = dot(a, vec4(1.0));

    vec4 color = vec4(0.0);

    if (sum > 0.0) {
        vec4 coords = mad(vec4( 0.0, a.r, 0.0,  -a.g), resolution.yyyy, texcoord.xyxy);
        color = mad(texture(colorTex, coords.xy), vec4(w.r), color);
        color = mad(texture(colorTex, coords.zw), vec4(w.g), color);

        coords = mad(vec4(-a.b,  0.0, a.a,  0.0), resolution.xxxx, texcoord.xyxy);
        color = mad(texture(colorTex, coords.xy), vec4(w.b), color);
        color = mad(texture(colorTex, coords.zw), vec4(w.a), color);

        color /= sum;
        color.a = 1.0;

        return color;
    } else {
        return texture(colorTex, texcoord);
    }
}

void main() {
    FragColor = NeighborhoodBlendingPS(coord);
}
