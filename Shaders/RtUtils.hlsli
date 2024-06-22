#include "RayCommon.hlsli"
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

float2 directionToSphericalEnvmap(float3 dir) {
  dir = normalize(dir);
  float2 uv = float2(atan2(dir.y, dir.x), asin(dir.z));
  uv /= float2(2.0 * PI, PI);
  uv += 0.5;
  uv.y = 1.0 - uv.y;
  return uv;
}
