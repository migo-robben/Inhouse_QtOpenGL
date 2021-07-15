#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in vec4 boneIds;
layout (location = 6) in vec4 weights;
layout (location = 7) in vec3 BlendShape1;
layout (location = 8) in vec3 BlendShape2;
layout (location = 9) in vec3 BlendShape3;
layout (location = 10) in vec3 BlendShape4;
layout (location = 11) in vec3 BlendShapeNormal1;
layout (location = 12) in vec3 BlendShapeNormal2;
layout (location = 13) in vec3 BlendShapeNormal3;
layout (location = 14) in vec3 BlendShapeNormal4;

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
uniform int BlendShapeNum;

out vec2 coord;
out vec3 WorldPos;
out vec3 Normal;

void main() {
    ivec4 BoneIds = ivec4(int(boneIds.x), int(boneIds.y), int(boneIds.z), int(boneIds.w));

    vec4 totalPosition = vec4(0.0f);
    vec3 localNormal = vec3(0.0f);
    vec3 newPos = aPos;
    vec3 newNor = aNormal;

    // compute blendshape influence
    for(int i = 0; i < BlendShapeNum; i++){
        if(i == 0){
            newPos += BlendShape1 * BlendShapeWeight1;
            newNor += BlendShapeNormal1 * BlendShapeWeight1;
        }
        if(i == 1){
            newPos += BlendShape2 * BlendShapeWeight2;
            newNor += BlendShapeNormal2 * BlendShapeWeight2;
        }
        if(i == 2){
            newPos += BlendShape3 * BlendShapeWeight3;
            newNor += BlendShapeNormal3 * BlendShapeWeight3;
        }
        if(i == 3){
            newPos += BlendShape4 * BlendShapeWeight4;
            newNor += BlendShapeNormal4 * BlendShapeWeight4;
        }
    }

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
    WorldPos = vec3(model * totalPosition);

    gl_Position =  projection * view * model * (totalPosition);

    coord = aCoord;
}