#include "NRICompatibility.hlsli"
#include "RtUtils.hlsli"

#include "layout.hlsli"

struct VisibilityContribution {
  float3 radiance; // Radiance at the point if light is visible
  float3 lightDir; // Direction to the light, to shoot shadow ray
  float lightDist; // Distance to the light (1e32 for infinite or sky)
  bool visible;    // true if in front of the face and should shoot shadow ray
};

VisibilityContribution DirectLight(in Ray r, in State state) {
  float3 Li = float3(0.0, 0.0, 0.0);
  float lightPdf;
  float3 lightContrib;
  float3 lightDir;
  float lightDist = 1e32;
  bool isLight = false;

  VisibilityContribution contrib;
  contrib.radiance = float3(0.0, 0.0, 0.0);
  contrib.visible = false;

  // sample Env

  float4 dirPdf; // = EnvSample(lightContrib);
  lightDir = dirPdf.xyz;
  lightPdf = dirPdf.w;

  return contrib;
}

[shader("closesthit")] void closest_hit(inout RayRayloadType payload
                                        : SV_RayPayload,
                                          in IntersectionAttributes intersect
                                        : SV_IntersectionAttributes) {
  // 得到实例索引
  uint instanceID = InstanceID();
  // 得到图元ID
  uint primitiveIndex = PrimitiveIndex();
  // 根据实例索引得到该实例的几何索引信息
  InstanceInfo instaInfo = instanceInfoBuffer[instanceID];

  uint u0 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 0];
  uint u1 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 1];
  uint u2 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 2];

  Vertex v0 = vertexBuffer[instaInfo.vertexOffset + u0];
  Vertex v1 = vertexBuffer[instaInfo.vertexOffset + u1];
  Vertex v2 = vertexBuffer[instaInfo.vertexOffset + u2];

  CameraUniform camUnifor = cameraUniform[0];

  float3 baryCentrics =
      float3(1.0 - intersect.barycentrics.x - intersect.barycentrics.y,
             intersect.barycentrics.x, intersect.barycentrics.y);

  float2 uvCoord = v0.texcoord * baryCentrics.x + v1.texcoord * baryCentrics.y +
                   v2.texcoord * baryCentrics.z;
  float3 vertPosition = v0.position * baryCentrics.x +
                        v1.position * baryCentrics.y +
                        v2.position * baryCentrics.z;
  vertPosition = mul((float3x3)ObjectToWorld3x4(), vertPosition).xyz;
  float3 vertNormal = v0.normal * baryCentrics.x + v1.normal * baryCentrics.y +
                      v2.normal * baryCentrics.z;
  vertNormal = mul((float3x3)ObjectToWorld3x4(), vertNormal).xyz;

  float3 vertTagent = normalize(v0.tangent.xyz * baryCentrics.x +
                                v1.tangent.xyz * baryCentrics.y +
                                v2.tangent.xyz * baryCentrics.z);
  vertTagent = normalize(mul((float3x3)ObjectToWorld3x4(), vertTagent).xyz);

  // 对应的那个实例的primitiveMaterialBuffer，之后根据primitiveID拿到材质索引
  uint materialIdx =
      primitiveIndexBuffers[instaInfo.primitiveInfoIdx][primitiveIndex];
  // 拿到材质
  MatUniform mat = matUniformBuffer[materialIdx];

  float3 baseColor = float3(0.5, 0.5, 0.5);
  if (mat.textureIndices[0] > -1) {
    Texture2D baseColorTexture = sceneTextures[mat.textureIndices[0]];
    baseColor = baseColorTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
  }

  float3 metallicRoughness = float3(0.0, 0.0, 0.0);
  if (mat.textureIndices[1] > -1) {
    Texture2D mrTexture = sceneTextures[mat.textureIndices[1]];
    metallicRoughness = mrTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
  }

  float ao = 0.0f;
  if (mat.textureIndices[2] > -1) {
    Texture2D aoTexture = sceneTextures[mat.textureIndices[2]];
    ao = aoTexture.SampleLevel(Sampler, uvCoord, 0.0).x;
  }

  float3 normal = normalize(vertNormal);
  if (mat.textureIndices[3] > -1) {
    Texture2D normalTexture = sceneTextures[mat.textureIndices[3]];
    float3 tagNormal = normalTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
    normal = normalMap(vertNormal, tagNormal);
  }

  float3 emissive = float3(0.0f, 0.0f, 0.0f);
  if (mat.textureIndices[4] > -1) {
    Texture2D emissiveTexture = sceneTextures[mat.textureIndices[4]];
    emissive = emissiveTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
  }

  //--------------------------------------------------------------------------

  float3 bitTangent = normalize(cross(vertTagent, normal));
  State state;
  state.position = vertPosition;
  state.normal = normal;
  state.tangent = vertTagent;
  state.bitangent = bitTangent;
  state.isEmitter = false;
  state.specularBounce = false;
  state.isSubsurface = false;
  state.ffnormal = dot(state.normal, payload.nextRayDirection) <= 0.0
                       ? state.normal
                       : -state.normal;
  CreateCoordinateSystem(state.ffnormal, state.tangent, state.bitangent);
  state.mat.specular = 1.0;
  state.mat.subsurface = 0.0;
  state.mat.specularTint = 1.0;
  state.mat.sheen = 0.0;
  state.mat.sheenTint = float3(0.0, 0.0, 0.0);
  state.texcoord = uvCoord;
  state.mat.albedo = baseColor;
  state.mat.emission = emissive;
  state.mat.roughness = max(metallicRoughness.y, 0.001);
  state.mat.metallic = max(metallicRoughness.z, 0.001);
  state.mat.transmission = mat.intensity.z;
  state.mat.transmission = 0.0;
  state.mat.ior = 1.83;
  state.eta = dot(state.normal, state.ffnormal) > 0.0 ? (1.0 / state.mat.ior)
                                                      : state.mat.ior;
  state.mat.anistropy = 0.0;
  float aspect = sqrt(1.0 - state.mat.anistropy * 0.9);
  state.mat.ax = max(0.001, state.mat.roughness / aspect);
  state.mat.ay = max(0.001, state.mat.roughness * aspect);

  state.mat.clearcoat = 0.0f;
  state.mat.clearcoatRoughness = 0.001;

  state.mat.sheenTint = float3(0.0, 0.0, 0.0);
  state.mat.sheen = 0.0;
  // add emissive light
  payload.directLight = float4(emissive, 1.0);

  // sampleEnv
  BsdfSampleRec bsdfSampleRec;
  bsdfSampleRec.L = float3(0.0, 0.0, 0.0);
  bsdfSampleRec.pdf = 0.0f;

  bsdfSampleRec.f =
      DisneySample(state, -payload.nextRayDirection, state.ffnormal,
                   bsdfSampleRec.L, bsdfSampleRec.pdf, payload.seed);

  if (bsdfSampleRec.pdf > 0.0) {
    payload.nextFactor *= bsdfSampleRec.f *
                          abs(dot(state.ffnormal, bsdfSampleRec.L)) /
                          bsdfSampleRec.pdf;
  } else {
    payload.level = 100;
    return;
  }

  // payload.directLight = float4(bsdfSampleRec.L, 1.0f);
  payload.nextRayDirection = bsdfSampleRec.L;
}