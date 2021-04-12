#version 460 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 nearPoint;
out vec3 farPoint;

out mat4 fragView;
out mat4 fragProj;

vec3 convertFromClipToViewSpace(vec2 posxy, float posz, mat4 view, mat4 projection) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);

    vec4 unprojectionPoint = viewInv * projInv * vec4(posxy, posz, 1.0);

    return unprojectionPoint.xyz / unprojectionPoint.w;
}

void main() {
    nearPoint = convertFromClipToViewSpace(aPos.xy, 0.0, view, projection);
    farPoint  = convertFromClipToViewSpace(aPos.xy, 1.0, view, projection);

    fragView = view;
    fragProj = projection;

    gl_Position = vec4(aPos, 1.0);
}
