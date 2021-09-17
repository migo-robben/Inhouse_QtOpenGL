#version 460 core
#extension GL_NV_shadow_samplers_cube : enable

in vec2 coord;
in vec3 WorldPos;
in vec3 Normal;
in vec3 displayColor;

out vec4 FragColor;

// lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// material parameters
uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D normalMap;

// SSAO
uniform sampler2D ssaoMap;

uniform vec3 camPos;

uniform int lightCounts;
uniform int useNormalMap;
uniform int useHDR;
uniform int useKullaContyApproxi = 1;
uniform int useSSAO = 1;

uniform vec2 res;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a_pow2 = pow(roughness, 2.0);
    float NdotH_pow2 = pow(max(dot(N, H), 0.0), 2.0);

    float nom = a_pow2;
    float denom = PI * pow((NdotH_pow2 * (a_pow2 - 1) + 1), 2.0);

    return nom/max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float k = pow((roughness+1), 2.0) / 8.0;

    return NdotV / (NdotV * (1-k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1*ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow((1.0 - cosTheta), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, coord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(coord);
    vec2 st2 = dFdy(coord);

    vec3 N  = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{
    vec3 albedo = texture(albedoMap, coord).rgb;
    float metallic = texture(metallicMap, coord).r;
    float roughness = texture(roughnessMap, coord).r;
    float ao = texture(aoMap, coord).r;

    vec3 N;
    if(useNormalMap == 1){
        N = getNormalFromMap();
    }else{
        N = normalize(Normal);
    }
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    for (int i=0; i<lightCounts; ++i)
    {
        // Step1
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        // Step2 Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        // Step3
        vec3 nominator = NDF * G * F;
        float denominator = 4 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0);
        vec3 specular_ss = nominator / (max(denominator, 0.001));

        // Step4
        vec3 kS = F;
        vec3 kDss = vec3(1.0) - kS;
        kDss *= 1.0 - metallic;

        // Step5
        float NdotL = max(dot(N, L), 0.0);

        // Step6
        Lo += (kDss * albedo / PI + specular_ss) * radiance * NdotL;
    }

    // Step7
    // TODO using useHDR control

    vec3 kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kDss = 1.0 - kS;
    kDss *= 1.0 - metallic;
    vec3 irradiance = textureCube(irradianceMap, N).rgb;
    vec3 diffuse    = irradiance * albedo;
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular_ss part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 radiance = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular_ss = radiance * (kS * brdf.x + brdf.y);


    // Multiple scattering - The Kulla-Conty Approximation
    vec3 FssEss = kS * brdf.x + brdf.y;
    float Ess = brdf.x + brdf.y;
    float Ems = 1.0 - Ess;
    vec3 Favg = F0 + (1.0 - F0) / 21.0;
    vec3 Fms = FssEss * Favg / (1.0 - (1.0 - Ess) * Favg);
    vec3 ambient = vec3(0.0);
    vec3 ambient_ms = vec3(0.0);

    vec3 Edss = 1.0 - (FssEss + Fms * Ems);
    vec3 kD = albedo * Edss;
    kD *= 1.0 - metallic;
    // ambient_ms = FssEss * radiance + (Fms * Ems) * irradiance;  // for metal
    ambient_ms = FssEss * radiance + (Fms * Ems + kD) * irradiance;  // for non-metal

    if(useKullaContyApproxi == 1){
        ambient = ambient_ms * ao;
    }else{
        ambient = (kDss * diffuse + specular_ss) * ao;
    }


    vec3 color = displayColor;
    color = color * (ambient + Lo);

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct

    // ssao
    float ssao;
    if(useSSAO==1){
        ssao = texture(ssaoMap, gl_FragCoord.xy/res.xy).r;
    }else{
        ssao = 1;
    }
    color = pow(color * ssao, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
