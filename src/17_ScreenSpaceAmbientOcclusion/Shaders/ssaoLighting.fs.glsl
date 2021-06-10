#version 460 core

out vec4 FragColor;
in vec2 Coords;

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;
uniform sampler2D mask;

struct Light {
    vec3 Position;
    vec3 Color;

    float Linear;
    float Quadratic;
};
uniform Light light;

void main() {
    // Retrieve data from g-buffer
    vec3 FragPos = texture(gPositionDepth, Coords).rgb;
    vec3 Normal = texture(gNormal, Coords).rgb;
    vec3 Diffuse = texture(gAlbedo, Coords).rgb;
    float AmbientOcclusion = texture(ssao, Coords).r;
    vec3 backgroundMask = vec3(texture(mask, Coords).r);

    // Then calculate lighting as usual
    vec3 ambient = vec3(0.3 * (1.0 - AmbientOcclusion)); // <-- this is where we use ambient occlusion
    vec3 lighting  = ambient;
    vec3 viewDir  = normalize(-FragPos); // Viewpos is (0.0.0)
    // Diffuse
    vec3 lightDir = normalize(light.Position - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * light.Color;
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 8.0);
    vec3 specular = light.Color * spec;
    // Attenuation
    float distance = length(light.Position - FragPos);
    float attenuation = 1.0 / (1.0 + light.Linear * distance + light.Quadratic * distance * distance);
    diffuse *= attenuation;
    specular *= attenuation;
    lighting += diffuse + specular;

    FragColor = vec4(lighting*backgroundMask, 1.0);
}
