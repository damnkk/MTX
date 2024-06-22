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

float PowerHeuristic(float a, float b) {
  float t = a * a;
  return t / (b * b + t);
}

float3 sampleMicrofacetBRDF(in float3 V, in float3 N, in float3 baseColor,
                            in float metallicness, in float fresnelReflect,
                            in float roughness, in float transmission,
                            in float ior, in float3 random,
                            out float3 nextFactor) {
  if (random.z < 0.5) {                    // non-specular light
    if ((2.0 * random.z) < transmission) { // transmitted light
      float3 forwardNormal = N;
      float frontFacing = dot(V, N);
      float eta = 1.0 / ior;
      if (frontFacing < 0.0) {
        forwardNormal = -N;
        eta = ior;
      }

      // important sample GGX
      // pdf = D * cos(theta) * sin(theta)
      float a = roughness * roughness;
      float theta =
          acos(sqrt((1.0 - random.y) / (1.0 + (a * a - 1.0) * random.y)));
      float phi = 2.0 * PI * random.x;

      float3 localH =
          float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      float3 H = normalMap(forwardNormal, localH);

      // compute L from sampled H
      float3 L = refract(-V, H, eta);

      // all required dot products
      float NoV = clamp(dot(forwardNormal, V), 0.0, 1.0);
      float NoL = clamp(dot(-forwardNormal, L), 0.0, 1.0); // reverse normal
      float NoH = clamp(dot(forwardNormal, H), 0.0, 1.0);
      float VoH = clamp(dot(V, H), 0.0, 1.0);

      // F0 for dielectics in range [0.0, 0.16]
      // default FO is (0.16 * 0.5^2) = 0.04
      float3 f0 = float3(0.16 * (fresnelReflect * fresnelReflect),
                         0.16 * (fresnelReflect * fresnelReflect),
                         0.16 * (fresnelReflect * fresnelReflect));
      // in case of metals, baseColor contains F0
      f0 = lerp(f0, baseColor, metallicness);

      float3 F = fresnelSchlick(VoH, f0);
      float D = D_GGX(NoH, roughness);
      float G = G_Smith(NoV, NoL, roughness);
      nextFactor = baseColor * (float3(1.0, 1.0, 1.0) - F) * G * VoH /
                   max((NoH * NoV), 0.001);

      nextFactor *= 2.0; // compensate for splitting diffuse and specular
      return L;

    } else { // diffuse light

      // important sampling diffuse
      // pdf = cos(theta) * sin(theta) / PI
      float theta = asin(sqrt(random.y));
      float phi = 2.0 * PI * random.x;
      // sampled indirect diffuse direction in normal space
      float3 localDiffuseDir =
          float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      float3 L = normalMap(N, localDiffuseDir);

      // half vector
      float3 H = normalize(V + L);
      float VoH = clamp(dot(V, H), 0.0, 1.0);

      // F0 for dielectics in range [0.0, 0.16]
      // default FO is (0.16 * 0.5^2) = 0.04
      float3 f0 = float3(0.16 * (fresnelReflect * fresnelReflect),
                         0.16 * (fresnelReflect * fresnelReflect),
                         0.16 * (fresnelReflect * fresnelReflect));
      // in case of metals, baseColor contains F0
      f0 = lerp(f0, baseColor, metallicness);
      float3 F = fresnelSchlick(VoH, f0);

      float3 notSpec =
          float3(1.0, 1.0, 1.0) - F;   // if not specular, use as diffuse
      notSpec *= (1.0 - metallicness); // no diffuse for metals

      nextFactor = notSpec * baseColor;
      nextFactor *= 2.0; // compensate for splitting diffuse and specular
      return L;
    }
  } else { // specular light

    // important sample GGX
    // pdf = D * cos(theta) * sin(theta)
    float a = roughness * roughness;
    float theta =
        acos(sqrt((1.0 - random.y) / (1.0 + (a * a - 1.0) * random.y)));
    float phi = 2.0 * PI * random.x;

    float3 localH =
        float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
    float3 H = normalMap(N, localH);
    float3 L = reflect(-V, H);

    // all required dot products
    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);

    // F0 for dielectics in range [0.0, 0.16]
    // default FO is (0.16 * 0.5^2) = 0.04
    float3 f0 = float3(0.16 * (fresnelReflect * fresnelReflect),
                       0.16 * (fresnelReflect * fresnelReflect),
                       0.16 * (fresnelReflect * fresnelReflect));
    // in case of metals, baseColor contains F0
    f0 = lerp(f0, baseColor, metallicness);

    // specular microfacet (cook-torrance) BRDF
    float3 F = fresnelSchlick(VoH, f0);
    float D = D_GGX(NoH, roughness);
    float G = G_Smith(NoV, NoL, roughness);
    nextFactor = F * G * VoH / max((NoH * NoV), 0.001);

    nextFactor *= 2.0; // compensate for splitting diffuse and specular
    return L;
  }
}
