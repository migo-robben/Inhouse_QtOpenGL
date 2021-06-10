#version 460 core

layout(location = 0) out vec4 gAlbedoSpec;
layout(location = 1) out vec4 gExpensiveNormal;
layout(location = 2) out vec4 gDepth;
layout(location = 3) out vec4 gExtraComponents;

in VS_OUT{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
    mat3 TBN2;
    vec3 wFragPos;
} fs_in;

uniform sampler2D albedoMap;
uniform sampler2D roughnessMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform samplerCube skybox;

uniform vec3 viewPos;

in vec4 PrevPosition;
in vec4 Position;

const float MAX_REFLECTION_LOD = 4.0;

void main() {
    vec3 camDir = normalize((fs_in.TBN * viewPos) - (fs_in.TBN * fs_in.wFragPos));

    vec2 texCoords = fs_in.TexCoords;

    vec4 Diffuse = texture(albedoMap, texCoords);
    vec3 r_tex = texture(roughnessMap, texCoords).xyz;
    float Roughness = clamp(r_tex.x, 0.05, 1.0);

    gAlbedoSpec.rgb = Diffuse.rgb;
    gAlbedoSpec.a = clamp(r_tex.z, 0.0, 1.0);

    gExpensiveNormal.rgb = normalize(fs_in.TBN * normalize(texture(normalMap, texCoords).rgb * 2.0 - 1.0));
    gExpensiveNormal.a = clamp(Roughness * Roughness, 0.0, 1.0);

    vec4 CubeMapcolor = vec4(0.0);
    CubeMapcolor = textureLod(skybox, reflect(normalize(fs_in.wFragPos - viewPos), normalize(gExpensiveNormal.rgb)), gExpensiveNormal.a * MAX_REFLECTION_LOD);
    gDepth = vec4(gl_FragCoord.z, CubeMapcolor.rgb * clamp(texture(metallicMap, texCoords).r, 0.04, 0.85));

    vec2 MetallicHeight = texture(metallicMap, texCoords).rg;
    gExtraComponents.x = min(MetallicHeight.x, 1.0);
    gExtraComponents.y = MetallicHeight.y;
    vec2 a = (Position.xy / Position.w) * 0.5 + 0.5;
    vec2 b = (PrevPosition.xy / PrevPosition.w) * 0.5 + 0.5;

    gExtraComponents.zw = vec2(a - b);
}