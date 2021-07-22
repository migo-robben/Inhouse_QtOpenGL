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
const int MAX_BLENDSHAPE = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

uniform int BlendShapeNum;
uniform int ScaleFactorX;
uniform int ScaleFactorY;
uniform int ScaleFactorZ;
uniform float BlendShapeWeight[MAX_BLENDSHAPE];
uniform vec3 BlendShapeSlice[MAX_BLENDSHAPE];
uniform sampler2DArray blendShapeMap;

uniform int Precision;

uniform sampler2D HeightMap1;

uniform int blendShapeOn = 1;

out vec2 coord;
out vec3 WorldPos;
out vec3 Normal;
out vec3 DebugColor;

int getRealVertexID4BS(int vs_id){
    int real_vs_id = vs_id;
    for(int i=0;i<BlendShapeNum;i++){
        int head = int(BlendShapeSlice[i].x);
        int end = int(BlendShapeSlice[i].y);
        int bs_pos = int(BlendShapeSlice[i].z);
        int diff = vs_id - head;
        if(diff >= head && diff <= end){
            real_vs_id = diff + bs_pos;
            break;
        }
    }
    return real_vs_id;
}

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
    int vertexId4BS = getRealVertexID4BS(gl_VertexID);
    int u = int(vertexId4BS % Precision);
    int v = int(vertexId4BS / Precision);
    ivec2 bsCoord = ivec2(u, v);
    vec2 bsCoordf = vec2(u/float(Precision), v/float(Precision));
    int u1 = int((vertexId4BS + halfIndex) % Precision);
    int v1 = int((vertexId4BS + halfIndex) / Precision);
    ivec2 bsCoord1 = ivec2(u1, v1);

    // Compute BlendShape pos and Normal
    for(int i=0; i < BlendShapeNum; i++){
        vec3 texBs = texelFetch(blendShapeMap, ivec3(bsCoord, i), 0).rgb * 2 - vec3(1.0);
        BsMapPos += vec3(texBs.r * ScaleFactorX, texBs.g * ScaleFactorY, texBs.b * ScaleFactorZ) * BlendShapeWeight[i];
        BsMapNor += (texelFetch(blendShapeMap, ivec3(bsCoord1, i), 0).rgb * 2 - vec3(1.0)) * BlendShapeWeight[i];
    }

    // Debug
    //vec4 bsdebugcolor = texture(blendShapeMap, vec3(bsCoordf, 0));
    //DebugColor = vec3(1,1,1);

    vec4 totalPosition = vec4(0.0f);
    vec3 localNormal = vec3(0.0f);
    vec3 newPos;
    vec3 newNor;
    if(blendShapeOn == 1){
        newPos = aPos + BsMapPos;
        newNor = aNormal + BsMapNor;
    }else{
        newPos = aPos;
        newNor = aNormal;
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
    vec4 finalPos = totalPosition + vec4(hnormal.xyz, 0.0);
    WorldPos = vec3(model * finalPos);

    //gl_Position =  projection * view * model * vec4(aPos, 1.0);
    gl_Position =  projection * view * model * finalPos;
}

/* ******************************************************************************
    vec2 uv = vec2(aCoord.x, aCoord.y);
    ivec2 texSize = textureSize(blendShapeMap1, 0);
    ivec2 texCoord = ivec2(uv * texSize);
*/