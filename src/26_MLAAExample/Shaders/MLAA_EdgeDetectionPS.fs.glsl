#version 460 core

#define MLAA_THRESHOLD 0.1

in vec2 coord;
in vec4 vOffset[2];
out vec4 FragColor;

// texture samplers
uniform sampler2D map;

vec4 MLAAColorEdgeDetectionPS(vec2 texcoord, vec4 offset[2], sampler2D colorTex) {

    // Calculate color deltas:
    vec4 delta = vec4(0.0);
    vec3 C = texture( colorTex, texcoord ).rgb; // 当前像素颜色

    // ----- 常规的边缘检测 ----- //
    vec3 Cleft = texture( colorTex, offset[0].xy ).rgb; // 左边的颜色
    vec3 t = abs( C - Cleft );
    delta.x = max( max( t.r, t.g ), t.b );

    vec3 Ctop = texture( colorTex, offset[0].zw ).rgb; // 上边的颜色
    t = abs( C - Ctop );
    delta.y = max( max( t.r, t.g ), t.b );

    // We do the usual threshold:
    vec4 edges = step( vec4(MLAA_THRESHOLD), delta.xyzw );
    // Then discard if there is no edge:
    if ( dot( edges, vec4(1.0) ) == 0.0 )
        discard;

    return vec4(edges.rg, 0.0, 1.0);
}

void main() {
    FragColor = MLAAColorEdgeDetectionPS(coord, vOffset, map);
}
