#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in vec4 boneIds;
layout (location = 6) in vec4 weights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 100;
const int MAX_BS_WEIGHT = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

uniform int BlendShapeNum;
uniform int iScaleFactor;
uniform float BlendShapeWeight[MAX_BS_WEIGHT];
uniform sampler2DArray blendShapeMap;

uniform int Precision;

uniform sampler2D HeightMap1;

out vec2 coord;
out vec3 WorldPos;
out vec3 Normal;
out vec3 DebugColor;

void main() {
    coord = aCoord;

    int halfIndex = Precision*Precision/2;
    ivec4 BoneIds = ivec4(int(boneIds.x), int(boneIds.y), int(boneIds.z), int(boneIds.w));
    vec3 BsMapPos = vec3(0.0);
    vec3 BsMapNor = vec3(0.0);
    float bias = 0.25;
    float height = 0.0;
    float hscale = 0.0;
    float bsscale = 0.0;

    // Height map
    height = hscale * ((texture2D(HeightMap1, aCoord).r) - bias);
    vec3 hnormal = vec3(aNormal.x*height, aNormal.y*height, aNormal.z*height);

    // BlendShape UV
    int u = int(gl_VertexID % Precision);
    int v = int(gl_VertexID / Precision);
    ivec2 bsCoord = ivec2(u, v);
    vec2 bsCoordf = vec2(u/float(Precision), v/float(Precision));
    int u1 = int((gl_VertexID+halfIndex) % Precision);
    int v1 = int((gl_VertexID+halfIndex) / Precision);
    ivec2 bsCoord1 = ivec2(u1, v1);

    // Compute BlendShape pos and Normal
    for(int i=0; i < BlendShapeNum; i++){
        BsMapPos += (texelFetch(blendShapeMap, ivec3(bsCoord, i), 0).rgb * 2 - vec3(1.0)) * iScaleFactor * BlendShapeWeight[i];
        BsMapNor += (texelFetch(blendShapeMap, ivec3(bsCoord1, i), 0).rgb * 2 - vec3(1.0)) * iScaleFactor * BlendShapeWeight[i];
    }

    // Debug
    //vec4 bsdebugcolor = texture(blendShapeMap, vec3(bsCoordf, 0));
    //DebugColor = vec3(bsdebugcolor);

    vec4 totalPosition = vec4(0.0f);
    vec3 localNormal = vec3(0.0f);
    vec3 newPos = aPos+BsMapPos;
    vec3 newNor = aNormal+BsMapNor;



    // compute bone influence
    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
    {
        if(BoneIds[i] == -1) {
            continue;
        }
        if(BoneIds[i] >= MAX_BONES)
        {
            totalPosition = vec4(newPos, 1.0);
            break;
        }
        vec4 localPosition = finalBonesMatrices[BoneIds[i]] * vec4(newPos, 1.0f);
        totalPosition += localPosition * weights[i];
        localNormal += mat3(finalBonesMatrices[BoneIds[i]]) * newNor * weights[i];
    }

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * localNormal);

    // compute world pos
    vec4 finalPos = totalPosition + vec4(hnormal.xyz, 0.0);
    WorldPos = vec3(model * finalPos);

    gl_Position =  projection * view * model * finalPos;
}

/* ******************************************************************************
    vec2 uv = vec2(aCoord.x, aCoord.y);
    ivec2 texSize = textureSize(blendShapeMap1, 0);
    ivec2 texCoord = ivec2(uv * texSize);
*/