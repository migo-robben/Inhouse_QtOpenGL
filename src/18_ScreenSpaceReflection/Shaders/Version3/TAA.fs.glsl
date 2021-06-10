#version 460

#pragma optionNV (unroll all)
#define USE_YCOCG
#define ZCMP_GT(a, b) (a > b)

layout(location = 0) out vec4 FragColor;

uniform sampler2D sCurrentFrame;
uniform sampler2D sLastFrame;
uniform sampler2D sVelocityBuffer;
uniform sampler2D sDepthBuffer;

in vec2 coord;

uniform vec2 iResolution;

float lerpAmount = 0.1;
int clampingKernelSize = 1;
float _FeedbackMin = 0.0;
float _FeedbackMax = 1.0;

const float FLT_EPS = 0.00000001;

float Luminance(vec3 rgb)
{
    // Algorithm from Chapter 10 of Graphics Shaders.
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    return dot(rgb, W);
}

vec3 find_closest_fragment_3x3(vec2 uv)
{
    vec2 texelSize = 1.0 / vec2(textureSize(sDepthBuffer, 0));
    vec2 dd = abs(texelSize);
    vec2 du = vec2(dd.x, 0.0);
    vec2 dv = vec2(0.0, dd.y);

    vec3 dtl = vec3(-1, -1, texture(sDepthBuffer, uv - dv - du).x);
    vec3 dtc = vec3( 0, -1, texture(sDepthBuffer, uv - dv).x);
    vec3 dtr = vec3( 1, -1, texture(sDepthBuffer, uv - dv + du).x);

    vec3 dml = vec3(-1, 0, texture(sDepthBuffer, uv - du).x);
    vec3 dmc = vec3( 0, 0, texture(sDepthBuffer, uv).x);
    vec3 dmr = vec3( 1, 0, texture(sDepthBuffer, uv + du).x);

    vec3 dbl = vec3(-1, 1, texture(sDepthBuffer, uv + dv - du).x);
    vec3 dbc = vec3( 0, 1, texture(sDepthBuffer, uv + dv).x);
    vec3 dbr = vec3( 1, 1, texture(sDepthBuffer, uv + dv + du).x);

    vec3 dmin = dtl;
    if (ZCMP_GT(dmin.z, dtc.z)) dmin = dtc;
    if (ZCMP_GT(dmin.z, dtr.z)) dmin = dtr;

    if (ZCMP_GT(dmin.z, dml.z)) dmin = dml;
    if (ZCMP_GT(dmin.z, dmc.z)) dmin = dmc;
    if (ZCMP_GT(dmin.z, dmr.z)) dmin = dmr;

    if (ZCMP_GT(dmin.z, dbl.z)) dmin = dbl;
    if (ZCMP_GT(dmin.z, dbc.z)) dmin = dbc;
    if (ZCMP_GT(dmin.z, dbr.z)) dmin = dbr;

    return vec3(uv + dd.xy * dmin.xy, dmin.z);
}

float NEAR = 0.1;
float FAR = 3000.0;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;  // Back to NDC
    return ((2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR)));
}

vec3 Gamma(in vec3 img) {
    return pow(img, vec3(1.0/2.2));
}

vec3 Degamma(in vec3 img) {
    return pow(img, vec3(2.2));
}

vec3 RGBToYCoCg( vec3 RGB )
{
    float Y = dot(RGB, vec3(  1, 2,  1 )) * 0.25;
    float Co= dot(RGB, vec3(  2, 0, -2 )) * 0.25 + ( 0.5 * 256.0/255.0 );
    float Cg= dot(RGB, vec3( -1, 2, -1 )) * 0.25 + ( 0.5 * 256.0/255.0 );
    return vec3(Y, Co, Cg);
}

vec3 YCoCgToRGB( vec3 YCoCg )
{
    float Y= YCoCg.x;
    float Co= YCoCg.y - ( 0.5 * 256.0 / 255.0 );
    float Cg= YCoCg.z - ( 0.5 * 256.0 / 255.0 );
    float R= Y + Co-Cg;
    float G= Y + Cg;
    float B= Y - Co-Cg;

    return vec3(R,G,B);
}

vec3 sampleTex(sampler2D tex, vec2 uv) {
    #ifdef USE_YCOCG
        return RGBToYCoCg(texture(tex, uv).rgb);
    #else
        return texture(tex, uv).rgb;
    #endif
}

vec3 resolve(vec3 tex) {
    #ifdef USE_YCOCG
        return YCoCgToRGB(tex);
    #else
        return tex;
    #endif
}

float weightLuminance(vec3 col0, vec3 col1) {

    float lum0 = col0.r;
    float lum1 = col1.r;
    float unbiased_diff = abs(lum0 - lum1) / max(lum0, max(lum1, 0.2));
    float unbiased_weight = 1.0 - unbiased_diff;
    float unbiased_weight_sqr = unbiased_weight * unbiased_weight;
    float k_feedback = mix(_FeedbackMin, _FeedbackMax, unbiased_weight_sqr);

    return k_feedback;
}

void main() {
    vec2 fragCoord = gl_FragCoord.xy;
    vec3 c_frag = find_closest_fragment_3x3(coord);
    vec2 velocity = texture(sVelocityBuffer, c_frag.xy).zw;
    float vs_dist = LinearizeDepth(c_frag.z);

    // get the neighborhood min / max from this frame's render
    vec3 center = sampleTex(sCurrentFrame, coord);
    vec3 minColor = center;
    vec3 maxColor = center;

    for (int iy = -clampingKernelSize; iy <= clampingKernelSize; ++iy)
    {
        for (int ix = -clampingKernelSize; ix <= clampingKernelSize; ++ix)
        {
            if (ix == 0 && iy == 0)
            continue;

            vec2 offsetUV = ((fragCoord + vec2(ix, iy)) / iResolution.xy);
            vec3 color = sampleTex(sCurrentFrame, offsetUV);
            minColor = min(minColor, color);
            maxColor = max(maxColor, color);
        }
    }

    // get last frame's pixel and clamp it to the neighborhood of this frame
    vec3 old = sampleTex(sLastFrame, coord - velocity);
    old = max(minColor, old);
    old = min(maxColor, old);

    float weight = weightLuminance(center, old);

    // interpolate from the clamped old color to the new color.
    vec3 pixelColor = resolve(mix(old, center, weight));

    FragColor = vec4(pixelColor, 1.0);

    FragColor.a = 1.0;
}
