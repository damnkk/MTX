#include "RayCommon.hlsli"
float radicalInverse(uint bits) {
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float3 OffsetRay(in float3 p, in float3 n) {
  const float intScale = 256.0f;
  const float floatScale = 1.0f / 65536.0f;
  const float origin = 1.0f / 32.0f;
  int3 of_i = int3(intScale * n.x, intScale * n.y, intScale * n.z);
  float3 p_i = float3(asfloat(asint(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
                      asfloat(asint(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
                      asfloat(asint(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));
  // return p_i;
  return float3(abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,
                abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,
                abs(p.z) < origin ? p.z + floatScale * n.z : p_i.z);
}

void CreateCoordinateSystem(in float3 N, out float3 Nt, out float3 Nb) {
  // http://www.pbr-book.org/3ed-2018/Geometry_and_Transformations/Vectors.html#CoordinateSystemfromaVector
  Nt = normalize(((abs(N.z) > 0.99999f)
                      ? float3(-N.x * N.y, 1.0f - N.y * N.y, -N.y * N.z)
                      : float3(-N.x * N.z, -N.y * N.z, 1.0f - N.z * N.z)));
  Nb = normalize(cross(Nt, N));
}

float2 hammersley(uint n, uint N) {
  return float2((float(n) + 0.5) / float(N), radicalInverse(n + 1u));
}

float3 normalMap(float3 vertexNormal, float3 tagNormal) {
  float3 tagent = normalize(cross(vertexNormal, float3(1.0f, 0.0f, 0.0f)));
  float3 bitTagent = normalize(cross(vertexNormal, tagent));
  return normalize(tagent * tagNormal.x + bitTagent * tagNormal.y +
                   vertexNormal * tagNormal.z);
}

float3 random_pcg3d(uint3 v) {
  v = v * 1664525u + 1013904223u;
  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  v ^= v >> 16u;
  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  return float3(v) * (1.0 / float(0xffffffffu));
}

float2 directionToSphericalEnvmap(float3 dir) {
  dir = normalize(dir);
  float2 uv = float2(atan2(dir.y, dir.x), asin(dir.z));
  uv /= float2(2.0 * PI, PI);
  uv += 0.5;
  uv.y = 1.0 - uv.y;
  return uv;
}
uint tea(in uint val0, in uint val1) {
  uint v0 = val0;
  uint v1 = val1;
  uint s0 = 0;

  for (uint n = 0; n < 16; n++) {
    s0 += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
  }

  return v0;
}

uint initRandom(in uint2 resolution, in uint2 screenCoord, in uint frame) {
  return tea(screenCoord.y * resolution.x + screenCoord.x, frame);
}

//-----------------------------------------------------------------------
// https://www.pcg-random.org/
//-----------------------------------------------------------------------
uint pcg(inout uint state) {
  uint prev = state * 747796405u + 2891336453u;
  uint word = ((prev >> ((prev >> 28u) + 4u)) ^ prev) * 277803737u;
  state = prev;
  return (word >> 22u) ^ word;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
uint2 pcg2d(uint2 v) {
  v = v * 1664525u + 1013904223u;
  v.x += v.y * 1664525u;
  v.y += v.x * 1664525u;
  v = v ^ (v >> 16u);
  v.x += v.y * 1664525u;
  v.y += v.x * 1664525u;
  v = v ^ (v >> 16u);
  return v;
}

uint3 pcg3d(uint3 v) {
  v = v * 1664525u + uint3(1013904223u, 1013904223u, 1013904223u);
  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  v ^= v >> uint3(16u, 16u, 16u);
  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  return v;
}

//-----------------------------------------------------------------------
// Generate a random float in [0, 1) given the previous RNG state
//-----------------------------------------------------------------------
float rand(inout uint seed) {
  uint r = pcg(seed);
  return asfloat(0x3f800000 | (r >> 9)) - 1.0f;
}

float2 rand2(inout uint prev) { return float2(rand(prev), rand(prev)); }

float GTR1(float NdotH, float a) {
  if (a >= 1.0)
    return M_1_OVER_PI; //(1.0 / PI);
  float a2 = a * a;
  float t = 1.0 + (a2 - 1.0) * NdotH * NdotH;
  return (a2 - 1.0) / (PI * log(a2) * t);
}

float GTR2(float NdotH, float a) {
  float a2 = a * a;
  float t = 1.0 + (a2 - 1.0) * NdotH * NdotH;
  return a2 / (PI * t * t);
}

float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay) {
  float a = HdotX / ax;
  float b = HdotY / ay;
  float c = a * a + b * b + NdotH * NdotH;
  return 1.0 / (PI * ax * ay * c * c);
}

float SmithG_GGX(float NdotV, float alphaG) {
  float a = alphaG * alphaG;
  float b = NdotV * NdotV;
  return 1.0 / (NdotV + sqrt(a + b - a * b));
}

float SmithG_GGX_aniso(float NdotV, float VdotX, float VdotY, float ax,
                       float ay) {
  float a = VdotX * ax;
  float b = VdotY * ay;
  float c = NdotV;
  return 1.0 / (NdotV + sqrt(a * a + b * b + c * c));
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float3 ImportanceSampleGTR2_aniso(float ax, float ay, float r1, float r2) {
  float phi = r1 * TWO_PI;

  float sinPhi = ay * sin(phi);
  float cosPhi = ax * cos(phi);
  float tanTheta = sqrt(r2 / (1 - r2));

  return float3(tanTheta * cosPhi, tanTheta * sinPhi, 1.0);
}

float SchlickFresnel(float u) {
  float m = clamp(1.0 - u, 0.0, 1.0);
  float m2 = m * m;
  return m2 * m2 * m; // pow(m,5)
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float3 CosineSampleHemisphere(float r1, float r2) {
  float3 dir;
  float r = sqrt(r1);
  float phi = TWO_PI * r2;
  dir.x = r * cos(phi);
  dir.y = r * sin(phi);
  dir.z = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.y * dir.y));

  return dir;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float3 UniformSampleHemisphere(float r1, float r2) {
  float r = sqrt(max(0.0, 1.0 - r1 * r1));
  float phi = TWO_PI * r2;

  return float3(r * cos(phi), r * sin(phi), r1);
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float3 UniformSampleSphere(float r1, float r2) {
  float z = 1.0 - 2.0 * r1;
  float r = sqrt(max(0.0, 1.0 - z * z));
  float phi = TWO_PI * r2;

  return float3(r * cos(phi), r * sin(phi), z);
}

float3 ImportanceSampleGTR1(float rgh, float r1, float r2) {
  float a = max(0.001, rgh);
  float a2 = a * a;

  float phi = r1 * TWO_PI;

  float cosTheta = sqrt((1.0 - pow(a2, 1.0 - r1)) / (1.0 - a2));
  float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
  float sinPhi = sin(phi);
  float cosPhi = cos(phi);

  return float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

float3 ImportanceSampleGTR2(float rgh, float r1, float r2) {
  float a = max(0.001, rgh);

  float phi = r1 * TWO_PI;

  float cosTheta = sqrt((1.0 - r2) / (1.0 + (a * a - 1.0) * r2));
  float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
  float sinPhi = sin(phi);
  float cosPhi = cos(phi);

  return float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

float DielectricFresnel(float cos_theta_i, float eta) {
  float sinThetaTSq = eta * eta * (1.0 - cos_theta_i * cos_theta_i);

  // Total internal reflection
  if (sinThetaTSq > 1.0)
    return 1.0;

  float cos_theta_t = sqrt(max(1.0 - sinThetaTSq, 0.0));

  float rs =
      (eta * cos_theta_t - cos_theta_i) / (eta * cos_theta_t + cos_theta_i);
  float rp =
      (eta * cos_theta_i - cos_theta_t) / (eta * cos_theta_i + cos_theta_t);

  return 0.5 * (rs * rs + rp * rp);
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float3 EvalDielectricRefraction(State state, float3 V, float3 N, float3 L,
                                float3 H, inout float pdf) {
  float F = DielectricFresnel(abs(dot(V, H)), state.eta);
  float D = GTR2(dot(N, H), state.mat.roughness);

  float denomSqrt = dot(L, H) * state.eta + dot(V, H);
  pdf = D * dot(N, H) * (1.0 - F) * abs(dot(L, H)) / (denomSqrt * denomSqrt);

  float G = SmithG_GGX(abs(dot(N, L)), state.mat.roughness) *
            SmithG_GGX(dot(N, V), state.mat.roughness);
  return state.mat.albedo * (1.0 - F) * D * G * abs(dot(V, H)) *
         abs(dot(L, H)) * 4.0 * state.eta * state.eta / (denomSqrt * denomSqrt);
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float3 EvalDielectricReflection(State state, float3 V, float3 N, float3 L,
                                float3 H, inout float pdf) {
  if (dot(N, L) < 0.0)
    return float3(0.0, 0.0, 0.0);

  float F = DielectricFresnel(dot(V, H), state.eta);
  float D = GTR2(dot(N, H), state.mat.roughness);

  pdf = D * dot(N, H) * F / (4.0 * dot(V, H));

  float G = SmithG_GGX(abs(dot(N, L)), state.mat.roughness) *
            SmithG_GGX(dot(N, V), state.mat.roughness);
  return state.mat.albedo * F * D * G;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float3 EvalDiffuse(State state, float3 Csheen, float3 V, float3 N, float3 L,
                   float3 H, inout float pdf) {
  if (dot(N, L) < 0.0)
    return float3(0.0, 0.0, 0.0);

  pdf = dot(N, L) * (1.0 / PI);

  float FL = SchlickFresnel(dot(N, L));
  float FV = SchlickFresnel(dot(N, V));
  float FH = SchlickFresnel(dot(L, H));
  float Fd90 = 0.5 + 2.0 * dot(L, H) * dot(L, H) * state.mat.roughness;
  float Fd = lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);
  float3 Fsheen = FH * state.mat.sheen * Csheen;
  return ((1.0 / PI) * Fd * (1.0 - state.mat.subsurface) * state.mat.albedo +
          Fsheen) *
         (1.0 - state.mat.metallic);
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float3 EvalSpecular(State state, float3 Cspec0, float3 V, float3 N, float3 L,
                    float3 H, inout float pdf) {
  if (dot(N, L) < 0.0)
    return float3(0.0, 0.0, 0.0);

  float D = GTR2_aniso(dot(N, H), dot(H, state.tangent),
                       dot(H, state.bitangent), state.mat.ax, state.mat.ay);
  pdf = D * dot(N, H) / (4.0 * dot(V, H));

  float FH = SchlickFresnel(dot(L, H));
  float3 F = lerp(Cspec0, float3(1.0, 1.0, 1.0), FH);
  float G =
      SmithG_GGX_aniso(dot(N, L), dot(L, state.tangent),
                       dot(L, state.bitangent), state.mat.ax, state.mat.ay);
  G *= SmithG_GGX_aniso(dot(N, V), dot(V, state.tangent),
                        dot(V, state.bitangent), state.mat.ax, state.mat.ay);
  return F * D * G;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float3 EvalClearcoat(State state, float3 V, float3 N, float3 L, float3 H,
                     inout float pdf) {
  if (dot(N, L) < 0.0)
    return float3(0.0, 0.0, 0.0);

  float D = GTR1(dot(N, H), state.mat.clearcoatRoughness);
  pdf = D * dot(N, H) / (4.0 * dot(V, H));

  float FH = SchlickFresnel(dot(L, H));
  float F = lerp(0.04, 1.0, FH);
  float G = SmithG_GGX(dot(N, L), 0.25) * SmithG_GGX(dot(N, V), 0.25);
  return float3(0.25 * state.mat.clearcoat * F * D * G,
                0.25 * state.mat.clearcoat * F * D * G,
                0.25 * state.mat.clearcoat * F * D * G);
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
float3 EvalSubsurface(State state, float3 V, float3 N, float3 L,
                      inout float pdf) {
  pdf = (1.0 / TWO_PI);

  float FL = SchlickFresnel(abs(dot(N, L)));
  float FV = SchlickFresnel(dot(N, V));
  float Fd = (1.0f - 0.5f * FL) * (1.0f - 0.5f * FV);
  return sqrt(state.mat.albedo) * state.mat.subsurface * (1.0 / PI) * Fd *
         (1.0 - state.mat.metallic) * (1.0 - state.mat.transmission);
}

float3 DisneySample(in State state, in float3 V, in float3 N, inout float3 L,
                    inout float pdf, inout uint seed) {
  state.isSubsurface = false;
  pdf = 0.0;
  float3 f = float3(0.0, 0.0, 0.0);
  float r1 = rand(seed);
  float r2 = rand(seed);

  float diffuseRatio = 0.5 * (1.0 - state.mat.metallic);
  float transWeight = (1.0 - state.mat.metallic) * state.mat.transmission;
  // transWeight = 1.0f;
  float3 Cdlin = state.mat.albedo;
  float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z;

  float3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : float3(1.0f, 1.0f, 1.0f);
  float3 Cspec0 =
      lerp(state.mat.specular * 0.08 *
               lerp(float3(1.0, 1.0, 1.0), Ctint, state.mat.specularTint),
           Cdlin, state.mat.metallic);
  float3 Csheen = state.mat.sheenTint;

  // BSDF
  if (rand(seed) < transWeight) {
    float3 H = ImportanceSampleGTR2(state.mat.roughness, r1, r2);
    H = state.tangent * H.x + state.bitangent * H.y + N * H.z;

    float3 R = reflect(-V, H);
    float F = DielectricFresnel(abs(dot(R, H)), state.eta);
    if (state.mat.tinwalled) {
      if (dot(state.ffnormal, state.normal) < 0.0) {
        F = 0.0;
      }
      state.eta = 1.001;
    }
    // Reflection/Total internal reflection
    if (rand(seed) < F) {
      L = normalize(R);
      f = EvalDielectricReflection(state, V, N, L, H, pdf);
    } else // Transmission
    {
      L = normalize(
          refract(-V, H, state.eta)); // refract light direction no mistake
      f = EvalDielectricRefraction(state, V, N, L, H, pdf);
    }
    f *= transWeight;
    pdf *= transWeight;
  } else {
    if (rand(seed) < diffuseRatio) {
      // subsurface transmission
      if (rand(seed) < state.mat.subsurface) {
        L = UniformSampleHemisphere(r1, r2);
        L = state.tangent * L.x + state.bitangent * L.y - N * L.z;
        f = EvalSubsurface(state, V, N, L, pdf);
        pdf *= state.mat.subsurface * diffuseRatio;
        state.isSubsurface = true;
      } else {
        L = CosineSampleHemisphere(r1, r2);
        L = state.tangent * L.x + state.bitangent * L.y + N * L.z;
        float3 H = normalize(L + V);
        f = EvalDiffuse(state, Csheen, V, N, L, H, pdf);
        pdf *= (1.0 - state.mat.subsurface) * diffuseRatio;
      }
    } else // specular
    {
      float primarySpecRatio = 1.0 / (1.0 + state.mat.clearcoat);
      // Sample primary specular lobe
      if (rand(seed) < primarySpecRatio) {
        float3 H =
            ImportanceSampleGTR2_aniso(state.mat.ax, state.mat.ay, r1, r2);
        H = state.tangent * H.x + state.bitangent * H.y + N * H.z;
        L = normalize(reflect(-V, H));

        f = EvalSpecular(state, Cspec0, V, N, L, H, pdf);
        pdf *= primarySpecRatio * (1.0 - diffuseRatio);
      } else {
        float3 H = ImportanceSampleGTR1(state.mat.clearcoatRoughness, r1, r2);
        H = state.tangent * H.x + state.bitangent * H.y + N * H.z;
        L = normalize(reflect(-V, H));

        f = EvalClearcoat(state, V, N, L, H, pdf);
        pdf *= (1.0 - primarySpecRatio) * (1.0 - diffuseRatio);
      }
    }

    f *= (1.0 - transWeight);
    pdf *= (1.0 - transWeight);
  }
  return f;
}
