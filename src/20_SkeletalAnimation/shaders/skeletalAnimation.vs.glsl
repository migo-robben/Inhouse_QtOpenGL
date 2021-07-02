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

out vec2 coord;

void main() {
    vec4 totalPosition = vec4(0.0f);
    ivec4 BoneIds = ivec4(int(boneIds.x), int(boneIds.y), int(boneIds.z), int(boneIds.w));
    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
    {
        if(BoneIds[i] == -1) {
            continue;
        }
        if(BoneIds[i] >= MAX_BONES)
        {
            totalPosition = vec4(aPos,1.0f);
            break;
        }
        vec4 localPosition = finalBonesMatrices[BoneIds[i]] * vec4(aPos,1.0f);
        totalPosition += localPosition * weights[i];
        vec3 localNormal = mat3(finalBonesMatrices[BoneIds[i]]) * aNormal;
    }

    mat4 viewModel = view * model;
    gl_Position =  projection * viewModel * totalPosition;

    coord = aCoord;
}