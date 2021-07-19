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
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

uniform float BlendShapeWeight1;
uniform float BlendShapeWeight2;
uniform float BlendShapeWeight3;
uniform float BlendShapeWeight4;
uniform float BlendShapeWeight5;
uniform float BlendShapeWeight6;
uniform float BlendShapeWeight7;
uniform float BlendShapeWeight8;

uniform int BlendShapeNum;
uniform int iScaleFactor;

uniform sampler2D blendShapeMap1;
uniform sampler2D blendShapeMap2;
uniform sampler2D blendShapeMap3;
uniform sampler2D blendShapeMap4;
uniform sampler2D blendShapeMap5;
uniform sampler2D blendShapeMap6;
uniform sampler2D blendShapeMap7;
uniform sampler2D blendShapeMap8;

uniform int Precision;

uniform sampler2D HeightMap1;

out vec2 coord;
out vec3 WorldPos;
out vec3 Normal;
out vec3 DebugColor;

void main() {
    coord = aCoord;

    ivec4 BoneIds = ivec4(int(boneIds.x), int(boneIds.y), int(boneIds.z), int(boneIds.w));
    vec3 BsMap = vec3(0.0);
    float bias = 0.25;
    float height = 0.0;
    float hscale = 0.0;
    float bsscale = 0.0;

    height = hscale * ((texture2D(HeightMap1, aCoord).r) - bias);
    vec3 hnormal = vec3(aNormal.x*height, aNormal.y*height, aNormal.z*height);

    int u = int(gl_VertexID / Precision);
    int v = int(gl_VertexID % Precision);
    ivec2 bsCoord = ivec2(u, v);
    vec2 bsCoordf = vec2(float(u)/Precision, float(v)/Precision);

    vec2 uv = vec2(aCoord.x, aCoord.y);
    ivec2 texSize = textureSize(blendShapeMap1, 0);
    ivec2 texCoord = ivec2(uv * texSize);
//    BsMap = (texelFetch(blendShapeMap1, bsCoord, 0).rgb * 2 - vec3(1.0)) * iScaleFactor * BlendShapeWeight1;
//    BsMap = texelFetch(blendShapeMap1, texCoord, 0);
    BsMap = (texture2D(blendShapeMap1, bsCoordf).rgb * 2.0 - vec3(1.0)) * float(iScaleFactor) * BlendShapeWeight1;
//    BsMap = texture2D(blendShapeMap1, aCoord).rgb;
//    if(gl_VertexID==0)
//        BsMap = vec3(1,0,0);
//    else
//        BsMap = vec3(0,0,0);

    DebugColor = vec3(BsMap);
    vec4 totalPosition = vec4(0.0f);
    vec3 localNormal = vec3(0.0f);
    vec3 newPos = aPos;
    vec3 newNor = aNormal;



    // compute blendshape influence
//    for(int i = 0; i < BlendShapeNum-BlendShapeNum; i++){
//        if(i == 0){
//            newPos += BlendShapeDeltaPos1 * BlendShapeWeight1;
//            newNor += BlendShapeDeltaNormal1 * BlendShapeWeight1;
//        }
//        if(i == 1){
//            newPos += BlendShapeDeltaPos2 * BlendShapeWeight2;
//            newNor += BlendShapeDeltaNormal2 * BlendShapeWeight2;
//        }
//        if(i == 2){
//            newPos += BlendShapeDeltaPos3 * BlendShapeWeight3;
//            newNor += BlendShapeDeltaNormal3 * BlendShapeWeight3;
//        }
//        if(i == 3){
//            newPos += BlendShapeDeltaPos4 * BlendShapeWeight4;
//            newNor += BlendShapeDeltaNormal4 * BlendShapeWeight4;
//        }
//    }

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
    vec4 finalPos = totalPosition + vec4(hnormal.xyz, 0.0) + vec4(BsMap, 0.0) * 0;
    WorldPos = vec3(model * finalPos);

    gl_Position =  projection * view * model * finalPos;
}