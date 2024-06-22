float radicalInverse(uint bits) {
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10; // / 0x100000000
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

static const float PI = 3.14159265358979;

float3 fresnelSchlick(float cosTheta, float3 F0) {
  return F0 + (1.0f - F0) * pow(1.0 - cosTheta, 5.0);
}

float D_GGX(float NoH, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float NoH2 = NoH * NoH;
  float b = (NoH2 * (alpha2 - 1.0) + 1.0);
  return alpha2 / (PI * b * b);
}

float G1_GGX_Schlick(float NdotV, float roughness) {
  float r = roughness; // original
  // float r = 0.5 + 0.5 * roughness; // Disney remapping
  float k = (r * r) / 2.0;
  float denom = NdotV * (1.0 - k) + k;
  return NdotV / denom;
}

float G_Smith(float NoV, float NoL, float roughness) {
  float g1_l = G1_GGX_Schlick(NoL, roughness);
  float g1_v = G1_GGX_Schlick(NoV, roughness);
  return g1_l * g1_v;
}

float2 directionToSphericalEnvmap(float3 dir) {

  dir = normalize(dir);
  float2 uv = float2(atan2(dir.y, dir.x), asin(dir.z));
  uv /= float2(2.0 * PI, PI);
  uv += 0.5;
  uv.y = 1.0 - uv.y;
  return uv;
}

float3 sampleMicrofacetBRDF(in float3 V, in float3 N, in float3 baseColor,
                            in float metallicness, in float fresnelReflect,
                            in float roughness, in float transmission,
                            in float ior, in float3 random,
                            out float3 nextFactor) {
  if (random.z < 0.5) {
    if ((2.0 * random.z) < transmission) {
      float3 forwardNormal = N;
      float frontFacting = dot(V, N);
      float eta = 1.0 / ior;
      if (frontFacting < 0.0) {
        forwardNormal = -N;
        eta = ior;
      }
      float a = roughness * roughness;
      float theta =
          acos(sqrt((1.0 - random.y) / (1.0 + (a * a - 1.0) * random.y)));
      float phi = 2.0 * PI * random.x;
    }
  }

  return float3(1.0f, 1.0f, 1.0f);
}
