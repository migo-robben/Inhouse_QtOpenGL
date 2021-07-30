#version 460 core

in vec2 coord;
in vec3 Normal;
in vec3 WorldPos;
in vec3 DebugColor;

out vec4 FragColor;

const int MAX_UDIM = 100;

uniform sampler2DArray diffuseUDIM;
uniform int UdimQuadrant[MAX_UDIM];
uniform int NumUdimQuadrant;

// texture samplers
uniform sampler2D colorMap;

void main() {

    // UDIM
    // convert assimp uv to maya
    vec2 aNewCoord = vec2(coord.x, -coord.y+1);
    int quadrantX, quadrantY;
    float threshold = 0.00001f;
    quadrantX = int(aNewCoord.x - threshold / 1) + 1;
    quadrantY = int(aNewCoord.y - threshold / 1);
    int udimLayour = -1;
    for(int i=0;i<NumUdimQuadrant;i++){
        int quadrant = 1000 + 000 + quadrantY*10 + quadrantX;
        if(UdimQuadrant[i] == quadrant){
            udimLayour = i;
        }
    }
    FragColor = texture(diffuseUDIM, vec3(coord, udimLayour));
}
