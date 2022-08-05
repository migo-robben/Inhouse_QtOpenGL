#version 460

layout (location = 0) out vec4 blendWeightTexture;
layout (location = 1) out vec4 blendWeightAlphaTexture;

in vec2 coord;
out vec4 FragColor;

uniform int MAXSEARCHSTEPS;

uniform vec2 resolution;

// texture samplers
uniform sampler2D edgesTex;
uniform sampler2D areaTex;

vec2 coordToColor(vec2 distance, float e1, float e2) {
      float areaSize = MAXSEARCHSTEPS * 5.0;
      vec2 pixcoord = MAXSEARCHSTEPS * round(4.0 * vec2(e1, e2)) + distance;
      pixcoord = vec2(pixcoord.x, areaSize - pixcoord.y - 1.0);

      return pixcoord / vec2(255.0);
}

vec4 mad(vec4 m, vec4 a, vec4 b) {
      return m * a + b;
}

float SearchXLeft(vec2 texcoord) {
      texcoord -= vec2(1.5, 0.0) * resolution;
      float e = 0.0;

      int i = 0;
      for (i = 0; i < MAXSEARCHSTEPS; i++) {
            // 寻找上部的边
            e = texture(edgesTex, texcoord, 0).g;
            if (e < 0.9) {
//                  if (e < 0.4) {
//                        break;
//                  } else {
//                        if ( texture(edgesTex, texcoord - vec2(0.5, 0.0) * resolution).g < 0.1 ) {
//                              e = 0.5;
//                        } else {
//                              e = 0.0;
//                        }
//                        break;
//                  }
                  break;
            }
            texcoord -= vec2(2.0, 0.0) * resolution;
      }

      return max(-2.0 * i - 2.0 * e, -2.0 * MAXSEARCHSTEPS);
}

float SearchXRight(vec2 texcoord) {
      texcoord += vec2(1.5, 0.0) * resolution;
      float e = 0.0;

      int i = 0;
      for (i = 0; i < MAXSEARCHSTEPS; i++) {
            // 寻找上部的边
            e = texture(edgesTex, texcoord, 0).g;
            if (e < 0.9) {
//                  if (e < 0.4) {
//                        break;
//                  } else {
//                        if ( texture(edgesTex, texcoord + vec2(0.5, 0.0) * resolution).g < 0.1 ) {
//                              e = 0.5;
//                        } else {
//                              e = 0.0;
//                        }
//                        break;
//                  }
                  break;
            }
            texcoord += vec2(2.0, 0.0) * resolution;
      }

      return min(2.0 * i + 2.0 * e, 2.0 * MAXSEARCHSTEPS);
}

float SearchYUp(vec2 texcoord) {
      texcoord += vec2(0.0, 1.5) * resolution;
      float e = 0.0;

      int i = 0;
      for (i = 0; i < MAXSEARCHSTEPS; i++) {
            e = texture(edgesTex, texcoord).r;
            if (e < 0.9) {
//                  if (e < 0.4) {
//                        break;
//                  } else {
//                        if ( texture(edgesTex, texcoord + vec2(0.0, 0.5) * resolution).r < 0.1 ) {
//                              e = 0.5;
//                        } else {
//                              e = 0.0;
//                        }
//                        break;
//                  }
                  break;
            }
            texcoord += vec2(0.0, 2.0) * resolution;
      }

      return min(2.0 * i + 2.0 * e, 2.0 * MAXSEARCHSTEPS);
}

float SearchYDown(vec2 texcoord) {
      texcoord -= vec2(0.0, 1.5) * resolution;
      float e = 0.0;

      int i = 0;
      for (i = 0; i < MAXSEARCHSTEPS; i++) {
            e = texture(edgesTex, texcoord).r;
            if (e < 0.9) {
//                  if (e < 0.4) {
//                        break;
//                  } else {
//                        if ( texture(edgesTex, texcoord - vec2(0.0, 0.5) * resolution).r < 0.1 ) {
//                              e = 0.5;
//                        } else {
//                              e = 0.0;
//                        }
//                        break;
//                  }
                  break;
            }
            texcoord -= vec2(0.0, 2.0) * resolution;
      }

      return max(-2.0 * i - 2.0 * e, -2.0 * MAXSEARCHSTEPS);
}

vec2 Area(vec2 distance, float e1, float e2) {
      float areaSize = MAXSEARCHSTEPS * 5.0;
      vec2 pixcoord = MAXSEARCHSTEPS * round(4.0 * vec2(e1, e2)) + distance;
      pixcoord = vec2(pixcoord.x, areaSize - pixcoord.y - 1.0);

      return texelFetch(areaTex, ivec2(pixcoord.x, pixcoord.y), 0).rg;
}

vec4 BlendingWeightCalculationPS(vec2 texcoord) {
      vec4 weights = vec4(0.0);

      vec2 e = texture(edgesTex, texcoord).rg;

      // 上边
      if (e.g > 0.0) {
            // Search distances to the left and to the right:
            vec2 d = vec2(SearchXLeft(texcoord), SearchXRight(texcoord));
            vec4 coords = mad(vec4(d.x, +0.25, d.y + 1.0, +0.25), resolution.xyxy, texcoord.xyxy);

            float e1 = texture(edgesTex, coords.xy).r;
            float e2 = texture(edgesTex, coords.zw).r;

            weights.rg = Area(abs(d), e1, e2);
      }

      // 左边
      if (e.r > 0.0) {
            // Search distances to the top and to the bottom:
            vec2 d = vec2(SearchYUp(texcoord), SearchYDown(texcoord));
            vec4 coords = mad(vec4(-0.25, d.x, -0.25, d.y - 1.0), resolution.xyxy, texcoord.xyxy);

            float e1 = texture(edgesTex, coords.xy).g;
            float e2 = texture(edgesTex, coords.zw).g;

            weights.ba = Area(abs(d), e1, e2);
      }

      return clamp(weights, vec4(0.0), vec4(1.0));
}

void main() {
      vec4 blendWeight = BlendingWeightCalculationPS(coord);
      blendWeightTexture = vec4(blendWeight.rgb, 1.0);
      blendWeightAlphaTexture.rgba = vec4(blendWeight.a, 0.0, 0.0, 1.0);
}
