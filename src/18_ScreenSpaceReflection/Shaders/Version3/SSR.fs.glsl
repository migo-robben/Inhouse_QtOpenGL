#version 460 core
#extension GL_NV_shadow_samplers_cube : enable

#define Scale vec3(.8, .8, .8)
#define K 19.19

uniform sampler2D gFinalImage;
uniform sampler2D gNormal;
uniform sampler2D gExtraComponents;
uniform sampler2D gDepth;
uniform sampler2D BRDF;
uniform sampler2D noiseTexture;
uniform sampler2D PreviousReflection;
uniform sampler2D gAlbedo;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 camPos;
uniform vec2 Resolution;
uniform vec2 LinMAD;
uniform float Time;
uniform int ID;
uniform bool isMoving;

uniform int maxSteps = 100;
uniform int numBinarySearchSteps = 10;
uniform float roughnessCutoff = 0.8;
uniform float depthCutoff = -1.05;

uniform float minRayStep = 0.01;
uniform float maxRayStep = 0.05;

const float rayIncrement = 1.04;
const float searchDist = 5;
const float searchDistInv = 0.2;
const float maxDDepth = 1.0;
const float maxDDepthInv = 1.0;
const float reflectionSpecularFalloffExponent = 3.0;

const float MAX_REFLECTION_LOD = 4.0;

float Roughness = 0.0;
float rayStep = 0.05;
float Metallic = 0.0;
float near = 0.01;
float far = 1000.0;

in vec2 Coords;
out vec4 FragColor;

vec3 PositionFromDepth(float depth);
vec4 RayCast(vec3 dir, inout vec3 hitCoord, out float dDepth);
vec3 BinarySearch(inout vec3 dir, inout vec3 hitCoord, inout float dDepth);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float saturate(float x);

float ViewSpaceZFromDepth(float d) {
    d = d * 2.0 - 1.0; // clip space
    return -1.0 / (LinMAD.x * d + LinMAD.y);
}

bool isSignificant(float dd) {
    return dd < 0.05 && dd > depthCutoff;
}

void main() {
    vec4 PrevReflection = texture(PreviousReflection, Coords);
    if (ID > 251) {
        FragColor = PrevReflection;
        return;
    }
    if (ID == 251) {
        FragColor = PrevReflection / 251;
        return;
    }

    rayStep = minRayStep;

    vec2 MetallicEmissive = texture(gExtraComponents, Coords).rg;
    vec2 texelSize = 1.0 / vec2(textureSize(gExtraComponents, 0));
    Metallic = MetallicEmissive.r;

    bool isPlastic = false;
    vec4 AlbedoSpec = texture(gAlbedo, Coords);
    vec4 NormalSpecular = texture(gNormal, Coords);
    vec3 worldNormal = normalize(NormalSpecular.rgb);
    float Spec = NormalSpecular.w;

    if (Metallic <= 0.1 || Spec > roughnessCutoff) {
        isPlastic = true;
    }

    vec3 viewNormal = vec3(view * vec4(worldNormal, 0.0));
    vec3 viewPos = PositionFromDepth(texture(gDepth, Coords).x);
    vec3 worldPos = vec3(inverse(view) * vec4(viewPos, 1.0));

    if (texture(gDepth, Coords).x > 0.9999f) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 viewDir = normalize(camPos - worldPos);

    Roughness = texture(gNormal, Coords).w;

    vec3 hitPos = viewPos;

    vec3 SSR;
    vec2 brdf = texture(BRDF, vec2(max(dot(worldNormal, viewDir), 0.0), Roughness)).rg;

    if (!isPlastic) {
        vec2 NoiseScale = Resolution / 4.0; // unused
        vec3 random = vec3(texture(noiseTexture, (Coords.xy * 1.0) + (1.0 - fract(Time))).rgb);
        random = dot(random, viewNormal) > 0.0 ? random : -random;
        float factor = Roughness * 0.20;
        vec3 hs = random * 2.0 - 1.0;
        vec3 jitt = hs * factor;
        vec3 reflected = normalize(reflect(normalize(viewPos), normalize(viewNormal)));
        float dDepth = 0.0;
        vec4 coords = RayCast(normalize(reflected + jitt) * max(minRayStep, -viewPos.z), hitPos, dDepth);
        vec2 centered_coords = abs(coords.xy * 2.0 - 1.0);
        float mixer = min(1.0, max(centered_coords.x, centered_coords.y));

        float screenEdgeFactor = float(mixer >= 0.9);
        vec2 pixelSize = 1.0 / textureSize(gFinalImage, 0).xy;
        vec3 samples = textureLod(gFinalImage, coords.xy, 0).rgb;

        SSR = mix(samples, texture(gDepth, Coords).gba, screenEdgeFactor);
    }
    else {
        SSR = texture(gDepth, Coords, 0).gba;
    }

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, pow(AlbedoSpec.rgb, vec3(1.0)), Metallic);
    vec3 Fresnel = fresnelSchlickRoughness(max(dot(worldNormal, viewDir), 0.0), F0, Roughness);

    FragColor = vec4(SSR * (1.0 - Roughness) * (Fresnel * brdf.x + brdf.y), 1.0);

    if (!isMoving && ID <= 250)
        FragColor = FragColor + PrevReflection;
}

vec3 PositionFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(Coords * 2.0 -1.0, z, 1.0);
    vec4 viewSpacePostion = inverse(projection) * clipSpacePosition;

    viewSpacePostion /= viewSpacePostion.w;

    return viewSpacePostion.xyz;
}

vec3 BinarySearch(inout vec3 dir, inout vec3 hitCoord, inout float dDepth) {
    float depth;

    vec4 projectedCoord;

    for (int i=0; i < numBinarySearchSteps; i++) {
        projectedCoord = projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

        depth = ViewSpaceZFromDepth(textureLod(gDepth, projectedCoord.xy, 0.0).x);
        dDepth = hitCoord.z - depth;

        dir *= 0.5;
        hitCoord += (dDepth > 0.0) ? dir : -dir;
    }

    projectedCoord = projection * vec4(hitCoord, 1.0);
    projectedCoord.xy /= projectedCoord.w;
    projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

    return vec3(projectedCoord.xy, dDepth);
}

vec4 RayCast(vec3 dir, inout vec3 hitCoord, out float dDepth)
{
    dir *= rayStep;

    float depth;
    int steps = 0;
    vec4 projectedCoord;

    hitCoord += dir;
    int miplevel = 0;
    for (int i=0; i < maxSteps; i++) {
        projectedCoord = projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

        depth = ViewSpaceZFromDepth(textureLod(gDepth, projectedCoord.xy, miplevel).x);

        if (depth > 1000.0) {
            continue;
        }

        dDepth = hitCoord.z - depth;

        if(isSignificant(dDepth) || (dir.z - dDepth) < 1.5) {
            do {
                depth = ViewSpaceZFromDepth(texture(gDepth, projectedCoord.xy, miplevel).x);
                dDepth = hitCoord.z - depth;

                if (miplevel > 0 && isSignificant(dDepth)) {
                    miplevel--;
                }
                else if(miplevel == 0 && isSignificant(dDepth)) {
                    return vec4(BinarySearch(dir, hitCoord, dDepth), 1.0);
                }
                else
                    break;
            } while(miplevel > 0 && isSignificant(dDepth));
            miplevel--;
        }

        hitCoord += dir;
        dir *= rayIncrement;
        steps++;
        miplevel++;
        miplevel = clamp(miplevel, 0, 5);
    }

    if (depth < 1000)
        return vec4(0.0);

    return vec4(BinarySearch(dir, hitCoord, dDepth), 1.0);
}

float saturate(float x) {
    return clamp(x, 0.0, 1.0);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    float F90 = saturate(dot(F0, vec3(50.0 * 0.33)));
    return F0 + (F90 - F0) * pow(1.0 - clamp(cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}