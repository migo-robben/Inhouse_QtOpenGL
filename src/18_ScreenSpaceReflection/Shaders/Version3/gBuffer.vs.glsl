#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
    mat3 TBN2;
    vec3 wFragPos;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 PrevModel;
uniform mat4 PrevView;

out vec4 PrevPosition;
out vec4 Position;

mat3 CreateTBNMatrix() {
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    return mat3(T, B, N);
}

mat3 CreateTBNMatrix2() {
    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    return mat3(T, B, N);
}

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);

    mat4 ViewMatrix = view;
    mat4 Projection = projection;

    vec4 viewPos = ViewMatrix * worldPos;

    vs_out.FragPos = viewPos.xyz;
    vs_out.wFragPos = worldPos.xyz;
    Position = projection * viewPos;
    gl_Position = Projection * viewPos;
    PrevPosition = projection * PrevView * PrevModel * vec4(aPos, 1.0);
    vs_out.TexCoords = aCoord;

    vs_out.TBN = CreateTBNMatrix();
    vs_out.Normal = vs_out.TBN * aNormal;
    vs_out.TBN2 = CreateTBNMatrix2();
}