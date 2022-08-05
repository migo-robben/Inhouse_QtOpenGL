#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aCoord;

uniform vec2 resolution;

out vec2 coord;
out vec4 vOffset[2];

void MLAAEdgeDetectionVS( vec2 texcoord ) {
    vOffset[0] = texcoord.xyxy + resolution.xyxy * vec4( -1.0, 0.0, 0.0,  1.0 );
    vOffset[1] = texcoord.xyxy + resolution.xyxy * vec4(  1.0, 0.0, 0.0, -1.0 );
}

void main() {
    coord = aCoord;
    MLAAEdgeDetectionVS( coord );
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
