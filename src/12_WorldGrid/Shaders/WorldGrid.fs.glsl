#version 460 core

#define NEAR 0.01
#define FAR 100.0

in vec3 nearPoint;
in vec3 farPoint;

in mat4 fragView;
in mat4 fragProj;

out vec4 FragColor;

vec4 grid(vec3 fragWorldPos, float scale) {
    vec2 coord = fragWorldPos.xz * scale; // Trick 1
    vec2 derivative = fwidth(coord); // The key to near big and far small
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative; // Trick 2 for AA
    float line = min(grid.x, grid.y);
    vec4 color = vec4(0.25, 0.25, 0.25, 1.0 - min(line-0.5, 1.0));

    return color;
}

float computeDepth(vec3 pos) {
    vec4 convertToClipSpace = fragProj * fragView * vec4(pos, 1.0);

    float NDCDepth = convertToClipSpace.z / convertToClipSpace.w;
    float screenSpaceDepth = 0.5 * NDCDepth + 0.5;

    return screenSpaceDepth;
}

float computeLinearDepth(vec3 pos) {
    vec4 convertToClipSpace = fragProj * fragView * vec4(pos, 1.0);

    float NDCDepth = convertToClipSpace.z / convertToClipSpace.w;
    float screenSpaceDepth = 0.5 * NDCDepth + 0.5;
    NDCDepth = screenSpaceDepth * 2.0 - 1.0; // back to between -1.0 ~ 1.0

    float linearDepth = (2.0 * NEAR * FAR) / (FAR + NEAR - NDCDepth * (FAR - NEAR));

    return linearDepth / FAR;
}

void main() {
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    vec3 fragWorldPos = nearPoint + t * (farPoint - nearPoint);

    float linearDepth = computeLinearDepth(fragWorldPos);
    float fading = max(0, (0.5 - linearDepth));

    FragColor = grid(fragWorldPos, 1.0) * float(t > 0.0) + grid(fragWorldPos, 0.1) * float(t > 0.0);
    FragColor.a *= fading;

    gl_FragDepth = computeDepth(fragWorldPos);
}
