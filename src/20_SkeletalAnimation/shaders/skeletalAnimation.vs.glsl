#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in vec4 aBlendShapeData;
layout (location = 6) in vec4 boneIds;
layout (location = 7) in vec4 weights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 120;
const int MAX_BLENDSHAPE = 200;
const int MAX_BLENDPART = 20;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

uniform int ScaleFactorX;
uniform int ScaleFactorY;
uniform int ScaleFactorZ;
uniform int NumBlendShapeWeight;
uniform int NumAnimation;
uniform vec2 BlendShapeWeight[MAX_BLENDSHAPE];
uniform vec4 BlendShapeSlice[MAX_BLENDSHAPE];
uniform vec2 BlendShapePrecision[MAX_BLENDSHAPE];
uniform sampler2DArray blendShapeMap[MAX_BLENDPART];

uniform sampler2D HeightMap1;

uniform int blendShapeOn = 1;

out vec2 coord;
out vec3 WorldPos;
out vec3 Normal;
out vec3 DebugColor;

ivec2 getBlendShapeIDs(int vs_id){
    int final_vs_id = -1;
    int final_map_id = -1;
    int bsNum = int(aBlendShapeData.x);
    for(int i=0;i<bsNum;i++){
        int head = int(BlendShapeSlice[i].x);
        int end = int(BlendShapeSlice[i].y);
        int bs_map_id = int(BlendShapeSlice[i].z);
        int bs_length = end - head;

        if(vs_id >= head && vs_id < end){
            final_vs_id = vs_id - head;
            final_map_id = bs_map_id;
            break;
        }
    }
    return ivec2(final_vs_id, final_map_id);
}

void main() {
    coord = aCoord;

    ivec4 BoneIds = ivec4(int(boneIds.x), int(boneIds.y), int(boneIds.z), int(boneIds.w));
    int blendShapeNum = int(aBlendShapeData.x);
    vec3 BsMapPos = vec3(0.0);
    vec3 BsMapNor = vec3(0.0);
    float bias = 0.25;
    float height = 0.0;
    float hscale = 0.0;
    float bsscale = 0.0;
    vec4 bsdebugcolor = vec4(0.0, 0.0, 0.0, 0.0);

    // Height map
    height = hscale * ((texture2D(HeightMap1, aCoord).r) - bias);
    vec3 hnormal = vec3(aNormal.x*height, aNormal.y*height, aNormal.z*height);

    // BlendShape
    int Precision = int(aBlendShapeData.z);
    ivec2 blendShapeIDs = getBlendShapeIDs(gl_VertexID);
    int bsVsID = blendShapeIDs.x;
    int bsMapID = blendShapeIDs.y;
    if( bsVsID != -1
        && blendShapeNum != 0
        //&& bsMapID == 1 // for debug single blendshape
    ){
        int halfIndex = Precision*Precision/2;
        int u = int(bsVsID % Precision);
        int v = int(bsVsID / Precision);
        ivec2 bsCoord = ivec2(u, v);
        vec2 bsCoordf = vec2(u/float(Precision), v/float(Precision));
        int u1 = int((bsVsID + halfIndex) % Precision);
        int v1 = int((bsVsID + halfIndex) / Precision);
        ivec2 bsCoord1 = ivec2(u1, v1);

        // Compute BlendShape pos and Normal
        for(int i=0; i < NumBlendShapeWeight; i++){
            vec2 bsWeightData = BlendShapeWeight[i];
            int bsMapID_iner = int(bsWeightData.x);

            if(bsMapID_iner == bsMapID){ // found startup here and break after deal once;
                for(int j=0;j<blendShapeNum;j++){
                    float bsWeight = BlendShapeWeight[i+j].y;

                    vec3 texBs = texelFetch(blendShapeMap[bsMapID], ivec3(bsCoord, j), 0).rgb * 2 - vec3(1.0);
                    BsMapPos += vec3(texBs.r * ScaleFactorX, texBs.g * ScaleFactorY, texBs.b * ScaleFactorZ) * bsWeight;
                    BsMapNor += (texelFetch(blendShapeMap[bsMapID], ivec3(bsCoord1, j), 0).rgb * 2 - vec3(1.0)) * bsWeight;
                }

                break;
            }
        }
    }

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
    if(NumAnimation != 0){
        // if current vertex that was't effected by any bone, we need restore the position of original
        bool noteffected = true;
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
            noteffected = false;
            vec4 localPosition = finalBonesMatrices[BoneIds[i]] * vec4(newPos, 1.0f);
            totalPosition += localPosition * weights[i];
            localNormal += mat3(finalBonesMatrices[BoneIds[i]]) * newNor * weights[i];
        }
        if(noteffected){
            totalPosition = vec4(newPos, 1.0);
        }
    }else{
        totalPosition = vec4(newPos, 1.0);
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