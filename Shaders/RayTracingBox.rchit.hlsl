#include "NRICompatibility.hlsli"
#include "RtUtils.hlsli"

#include "layout.hlsli"

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

  // 对应的那个实例的primitiveMaterialBuffer，之后根据primitiveID拿到材质索引
  uint materialIdx =
      primitiveIndexBuffers[instaInfo.primitiveInfoIdx][primitiveIndex];
  // 拿到材质
  MatUniform mat = matUniformBuffer[materialIdx];

  Texture2D baseColorTexture = sceneTextures[mat.textureIndices[0]];
  Texture2D mrTexture = sceneTextures[mat.textureIndices[1]];
  Texture2D emissiveTexture = sceneTextures[mat.textureIndices[4]];
  Texture2D aoTexture = sceneTextures[mat.textureIndices[2]];

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

  float3 baseColor = baseColorTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;

  float3 normal = vertNormal;
  if (mat.textureIndices[3] > -1) {
    Texture2D normalTexture = sceneTextures[mat.textureIndices[3]];
    float3 tagNormal = normalTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
    normal = normalMap(vertNormal, tagNormal);
  }
  normal = mul((float3x3)ObjectToWorld3x4(), normal).xyz;

  float3 metallicRoughness = float3(0.0, 0.0, 0.0);
  if (mat.textureIndices[1] > -1) {
    Texture2D mrTexture = sceneTextures[mat.textureIndices[1]];
    metallicRoughness = mrTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
  }

  float3 mr = mrTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
  float3 emissive = emissiveTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
  float ao = aoTexture.SampleLevel(Sampler, uvCoord, 0.0);

  float3 random = random_pcg3d(
      uint3(DispatchRaysIndex().xy, RTConstant.curFrameCount + payload.level));

  if (mat.textureIndices[4] > -1) {
    Texture2D emissiveTexture = sceneTextures[mat.textureIndices[4]];
    payload.directLight = emissiveTexture.SampleLevel(Sampler, uvCoord, 0.0);
  }
}