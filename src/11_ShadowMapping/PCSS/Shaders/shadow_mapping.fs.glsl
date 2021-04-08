#version 460

#define NEAR 0.1
#define FAR 50.0
#define LIGHTSIZE 10.0
#define LIGHTSEARCHWIDTHSCALE 500.0

// Shadow map related variables
#define NUM_SAMPLES 50
#define BLOCKER_SEARCH_NUM_SAMPLES NUM_SAMPLES
#define PCF_NUM_SAMPLES NUM_SAMPLES
#define NUM_RINGS 10

#define EPS 1e-3
#define PI 3.141592653589793
#define PI2 6.283185307179586

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
    vec4 PosLightSpace;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform int MODE;

float rand_1to1(float x ) {
    // -1 -1
    return fract(sin(x)*10000.0);
}

float rand_2to1(vec2 uv ) {
    // 0 - 1
    const float a = 12.9898, b = 78.233, c = 43758.5453;
    float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
    return fract(sin(sn) * c);
}

vec2 poissonDisk[NUM_SAMPLES];

void poissonDiskSamples( const in vec2 randomSeed ) {

    float ANGLE_STEP = PI2 * float( NUM_RINGS ) / float( NUM_SAMPLES );
    float INV_NUM_SAMPLES = 1.0 / float( NUM_SAMPLES );

    float angle = rand_2to1( randomSeed ) * PI2;
    float radius = INV_NUM_SAMPLES;
    float radiusStep = radius;

    for( int i = 0; i < NUM_SAMPLES; i ++ ) {
        poissonDisk[i] = vec2( cos( angle ), sin( angle ) ) * pow( radius, 0.75 );
        radius += radiusStep;
        angle += ANGLE_STEP;
    }
}

// Using similar triangles from the surface point to the area light
float SearchRegionRadiusUV(float z) {
    float blockerSearchScale = 1.0f;
    return blockerSearchScale * (z - NEAR) / z * LIGHTSIZE;
}

// Returns average blocker depth in the search region, as well as the number of found blockers.
// Blockers are defined as shadow-map samples between the surface point and the light.
void FindBlocker(out float accum_blocker_depth,
    out float num_blockers,
    vec2 uv,
    float search_region_radius_uv,
    float currentDepth,
    float bias) {

    poissonDiskSamples(uv);
    accum_blocker_depth = 0.0;
    num_blockers        = 0.0;

    float biased_depth  = currentDepth - bias;

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 offset = poissonDisk[i];
        offset *= search_region_radius_uv/LIGHTSEARCHWIDTHSCALE;

        float pcfDepth = texture(shadowMap, uv + offset).r;

        if (pcfDepth < biased_depth)
        {
            accum_blocker_depth += pcfDepth;
            num_blockers++;
        }
    }
}

float z_clip_to_eye(float z)
{
    z = z * 2.0 - 1.0;
    return 2.0 * FAR * NEAR / (FAR + NEAR - z * (FAR - NEAR));
}

// Using similar triangles between the area light, the blocking plane and the surface point
float PenumbraRadiusUV(float zReceiver, float zBlocker)
{
    return abs(zReceiver - zBlocker) / zBlocker;
}

// Project UV size to the near plane of the light
float ProjectToLightUV(float penumbra_radius, float z)
{
    return penumbra_radius * LIGHTSIZE * NEAR / z;
}

float pcfPoissonFilter(vec2 uv, float z0, float bias, float filter_radius_uv)
{
    float sum = 0.0;

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 offset = poissonDisk[i];
        offset *= filter_radius_uv;
        float shadow_map_depth = texture(shadowMap, uv + offset).r;

        sum += shadow_map_depth < (z0 - bias) ? 1.0 : 0.0;
    }
    return sum / float(NUM_SAMPLES);
}

float PcssFilter(vec2 uv, float z_vs, float currentDepth) {
    float bias = 0.00001;
    // ------------------------
    // STEP 1: blocker search
    // ------------------------
    float search_region_radius_uv = SearchRegionRadiusUV(z_vs);

    float accum_blocker_depth, num_blockers;
    FindBlocker(accum_blocker_depth, num_blockers, uv, search_region_radius_uv, currentDepth, bias);

    float avg_blocker_depth = accum_blocker_depth / num_blockers;

    // ------------------------
    // STEP 2: penumbra size
    // ------------------------
    float avg_blocker_depth_vs = z_clip_to_eye(avg_blocker_depth);
    float penumbra_radius      = PenumbraRadiusUV(z_vs, avg_blocker_depth_vs);
    float filter_radius        = ProjectToLightUV(penumbra_radius, z_vs);

    // ------------------------
    // STEP 3: filtering
    // ------------------------
    return pcfPoissonFilter(uv, currentDepth, bias, filter_radius);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec4 posLightSpace) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    //
    vec4 pos_vs = posLightSpace;
    pos_vs.xyz /= pos_vs.w;

    float shadow = PcssFilter(projCoords.xy, -(pos_vs.z), currentDepth);

    return shadow;
}

void main() {
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(0.75);

    // ambient
    vec3 ambient = 0.3 * color;

    // diffuse
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;

    // calculate shadow
    float visibility = ShadowCalculation(fs_in.FragPosLightSpace, fs_in.PosLightSpace);

    // lighting
    vec3 lighting = (ambient + (1.0 - visibility) * (diffuse + specular)) * color;
    lighting = pow(lighting, vec3(0.4545));

    FragColor = vec4(lighting, 1.0);
}