#version 460 core

in VS_OUT {
    vec3 FragPos;
    vec2 coord;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

out vec4 FragColor;

// texture samplers
uniform sampler2D AlbedoMap;
uniform sampler2D NormalMap;
uniform sampler2D SpecularMap;

uniform float diffuseWeight;
uniform float specularWeight;

void main() {
    vec3 normal = texture(NormalMap, fs_in.coord).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    vec3 color = texture(AlbedoMap, fs_in.coord).rgb;
    vec3 ambient = diffuseWeight * color;

    vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;

    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 600.0);
    vec3 specular = texture(SpecularMap, fs_in.coord).rgb * spec * vec3(specularWeight);

    FragColor = vec4(ambient + diffuse + specular, 1.0);
//    FragColor = texture(SpecularMap, fs_in.coord);
}
