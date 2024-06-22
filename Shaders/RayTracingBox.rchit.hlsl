#include "NRICompatibility.hlsli"
#include "RayCommon.hlsli"

NRI_RESOURCE(StructuredBuffer<MatUniform>, matUniformBuffer, t, 0, 1);
NRI_RESOURCE(StructuredBuffer<Vertex>, vertexBuffer, t, 1, 1);
NRI_RESOURCE(StructuredBuffer<uint>, indexBuffer, t, 2, 1);
NRI_RESOURCE(StructuredBuffer<InstanceInfo>, instanceInfoBuffer, t, 3, 1);
NRI_RESOURCE(Texture2D<float4>, sceneTextures[], t, 0, 2);
NRI_RESOURCE(StructuredBuffer<uint>, primitiveIndexBuffers[], t, 0, 4);
NRI_RESOURCE(SamplerState, Sampler, s, 4, 1);
NRI_RESOURCE(StructuredBuffer<CameraUniform>, cameraUniform, t, 2, 0);
NRI_PUSH_CONSTANTS(PushConstant, RTConstant, 0);
struct IntersectionAttributes {
  float2 barycentrics;
};

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
  // 对应的那个实例的primitiveMaterialBuffer，之后根据primitiveID拿到材质索引
  uint materialIdx =
      primitiveIndexBuffers[instaInfo.primitiveInfoIdx][primitiveIndex];
  // 拿到材质
  MatUniform mat = matUniformBuffer[materialIdx];

  uint u0 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 0];
  uint u1 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 1];
  uint u2 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 2];

  Vertex v0 = vertexBuffer[instaInfo.vertexOffset + u0];
  Vertex v1 = vertexBuffer[instaInfo.vertexOffset + u1];
  Vertex v2 = vertexBuffer[instaInfo.vertexOffset + u2];

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
  float3 vertNormal = v0.normal * baryCentrics.x + v1.normal * baryCentrics.y +
                      v2.normal * baryCentrics.z;

  float3 baseColor = baseColorTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
  float3 normal = vertNormal;
  if (mat.textureIndices[3] > -1) {
    Texture2D normalTexture = sceneTextures[mat.textureIndices[3]];
    float3 tagNormal = normalTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
    normal = normalMap(vertNormal, tagNormal);
  }
  normal = mul(normal, (float3x3)ObjectToWorld3x4()).xyz;

  float3 mr = mrTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
  float3 emissive = emissiveTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
  float ao = aoTexture.SampleLevel(Sampler, uvCoord, 0.0);

  float3 random = random_pcg3d(
      uint3(DispatchRaysIndex().xy, RTConstant.curFrameCount + payload.level));

  float3 nextFactor = float3(0.0f, 0.0f, 0.0f);

  payload.directLight = float4(random, 1.0);
}