#version 460 core
#extension GL_NV_shadow_samplers_cube : enable

in vec2 coord;
in vec3 WorldPos;
in vec3 Normal;

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
uniform sampler2D uEavgLut;

uniform vec3 camPos;
uniform float roughnessMount;
uniform bool computePointLight;
uniform bool environmentCompensation;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom/max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float k = pow((roughness + 1.0), 2.0) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
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

vec3 AverageFresnel(vec3 r, vec3 g) {
    return vec3(0.087237) + 0.0230685*g - 0.0864902*g*g + 0.0774594*g*g*g
    + 0.782654*r - 0.136432*r*r + 0.278708*r*r*r
    + 0.19744*g*r + 0.0360605*g*g*r - 0.2586*g*r*r;
}

vec3 multipleScatteringBRDF(float NdotL, float NdotV, float rough) {
    vec3 albedo = texture2D(albedoMap, coord).rgb;

    vec3 E_o = vec3(texture(brdfLUT, vec2(NdotL, rough)).r + texture(brdfLUT, vec2(NdotL, rough)).b);
    vec3 E_i = vec3(texture(brdfLUT, vec2(NdotV, rough)).r + texture(brdfLUT, vec2(NdotV, rough)).b);

    vec3 E_avg = texture(uEavgLut, vec2(0, rough)).rgb;

    // Gold
    vec3 edgetint = vec3(0.98616, 0.96586, 0.88734);
    vec3 F_avg = AverageFresnel(albedo, edgetint);

    vec3 f_add = (F_avg * F_avg * E_avg) / (1.0 - F_avg * (1.0 - E_avg));
    vec3 f_ms = (1.0 - E_o) * (1.0 - E_i) / (PI * (1.0 - E_avg));

    return f_add * f_ms;
}

void main()
{
    vec3 albedo = texture(albedoMap, coord).rgb;
    float metallic = texture(metallicMap, coord).r;
    float roughness = roughnessMount;
    float ao = texture(aoMap, coord).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N);
    float NdotV = max(dot(N, V), 0.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // ----------- Compute Multiple Scatter From Light Start ----------- //
    // ----------------------------------------------------------------- //
    vec3 colorFromLight = vec3(0.0);
    for (int i = 0; i < 4; i++) {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 Fresnel = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        vec3 nominator = NDF * G * Fresnel;
        float denominator = 4 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0);
        vec3 Fmicro = nominator / (max(denominator, 0.001));

        float NdotL = max(dot(N, L), 0.0);
        vec3 Fms = multipleScatteringBRDF(NdotL, NdotV, roughness);

        vec3 BRDF = Fmicro + Fms;

        vec3 kDss = vec3(1.0) - Fresnel;
        kDss *= 1.0 - metallic;

        colorFromLight += (kDss * albedo / PI + BRDF) * radiance * NdotL;
    }
    if (computePointLight) {
    }
    else {
        colorFromLight = vec3(0.0);
    }

    // ---------- Compute Multiple Scatter From Ambient Start ---------- //
    // ----------------------------------------------------------------- //
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;

    vec2 f_ab = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 FssEss = kS * f_ab.x + f_ab.y;

    // Prefiltered radiance
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 radiance = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;
    // Cosine-weighted irradiance
    vec3 irradiance = textureCube(irradianceMap, N).rgb;

    // Multiple scattering
    vec3 colorAmbient = vec3(0.0);
    if (environmentCompensation) {
        float Ess = f_ab.x + f_ab.y;
        float Ems = 1.0 - Ess;
        vec3 Favg = F0 + (1.0 - F0) / 21.0;
        vec3 Fms = FssEss * Favg / (1.0 - (1.0 - Ess) * Favg);

        vec3 Edss = 1.0 - (FssEss + Fms * Ems);
        vec3 kD = albedo * Edss;
        kD *= 1.0 - metallic;
        colorAmbient = FssEss * radiance + (Fms * Ems + kD) * irradiance;
    }
    else {
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;
        vec3 diffuse = irradiance * albedo;

        vec3 specular = radiance * (kS * f_ab.x + f_ab.y);
        colorAmbient = (kD * diffuse + specular);
    }

    vec3 ambient = colorAmbient * ao;
    // --------------------------------------------------------------- //
    // ---------- Compute Multiple Scatter From Ambient End ---------- //

    vec3 color = ambient + colorFromLight;

    // gamma correct
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
