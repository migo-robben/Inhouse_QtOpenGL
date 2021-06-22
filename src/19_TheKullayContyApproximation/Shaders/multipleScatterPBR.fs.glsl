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

uniform vec3 camPos;

// material parameters
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

uniform bool isMetal;
uniform bool computePointLight;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a_pow2 = pow(roughness, 2.0);
    float NdotH_pow2 = pow(max(dot(N, H), 0.0), 2.0);

    float nom = a_pow2;
    float denom = PI * pow((NdotH_pow2 * (a_pow2 - 1) + 1), 2.0);

    return nom/max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float k = pow((roughness+1), 2.0) / 8.0;

    return NdotV / (NdotV * (1-k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1*ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow((1.0 - cosTheta), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float AverageEnergy(float rough) {
    float smoothness = 1.0 - rough;
    float r = -0.0761947 - 0.383026 * smoothness;
    r = 1.04997 + smoothness * r;
    r = 0.409255 + smoothness * r;

    return min(0.999, r);
}

vec3 AverageFresnel(vec3 specularColor) {
    return specularColor + (vec3(1.0) - specularColor) * (1.0 / 21.0);
}

vec3 multipleScatteringBRDF(float NdotL, float NdotV, float rough) {
    vec2 sampleE_o = texture(brdfLUT, vec2(NdotV, rough)).rg;
    float E_o = sampleE_o.x + sampleE_o.y;
    float oneMinusE_o = 1.0 - E_o;
    vec2 sampleE_i = texture(brdfLUT, vec2(NdotL, rough)).rg;
    float oneMinusE_i = 1.0 - (sampleE_i.x + sampleE_i.y);

    float Eavg = AverageEnergy(rough);
    float oneMinusEavg = 1.0 - Eavg;
    vec3 Favg = AverageFresnel(vec3(0.04));

    float brdf = (oneMinusE_o * oneMinusE_i) / (PI * oneMinusEavg);
    vec3 energyScale = (Favg * Favg * Eavg) / (vec3(1.0) - Favg * oneMinusEavg);

    return brdf * energyScale;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // ----------- Compute Multiple Scatter From Light Start ----------- //
    // ----------------------------------------------------------------- //
    vec3 colorFromLight = vec3(0.0);
    for (int i = 0; i < 4; ++i) {
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
        vec3 specular_ss = nominator / (max(denominator, 0.001));

        vec3 msBRDF = multipleScatteringBRDF(
        max(dot(N, L), 0.0),
        max(dot(N, V), 0.0),
        roughness);

        vec3 kDss = vec3(1.0) - Fresnel;
        kDss *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);

        colorFromLight += (kDss * albedo / PI + specular_ss + msBRDF*0.0) * radiance * NdotL;
    }
    if (computePointLight) {
    }
    else {
        colorFromLight = vec3(0.0);
    }
    // ------------ Compute Multiple Scatter From Light End ------------ //
    // ----------------------------------------------------------------- //


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
    float Ess = f_ab.x + f_ab.y;
    float Ems = 1.0 - Ess;
    vec3 Favg = F0 + (1.0 - F0) / 21.0;
    vec3 Fms = FssEss * Favg / (1.0 - (1.0 - Ess) * Favg);

    vec3 colorAmbientMS = vec3(0.0);
    if (isMetal) {
        colorAmbientMS = FssEss * radiance + Fms * Ems * irradiance;
    }
    else {
        vec3 Edss = 1.0 - (FssEss + Fms * Ems);
        vec3 kD = albedo * Edss;
        kD *= 1.0 - metallic;
        colorAmbientMS = FssEss * radiance + (Fms * Ems + kD) * irradiance;
    }

    vec3 ambient = colorAmbientMS * ao;
    // --------------------------------------------------------------- //
    // ---------- Compute Multiple Scatter From Ambient End ---------- //

    // ----- Final Result ----- //
    vec3 color = ambient + colorFromLight;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
