#include "NRICompatibility.hlsli"
#include "RayCommon.hlsli"

NRI_RESOURCE(StructuredBuffer<MatUniform>, matUniformBuffer, t, 0, 1);
NRI_RESOURCE(StructuredBuffer<Vertex>, vertexBuffer, t, 1, 1);
NRI_RESOURCE(StructuredBuffer<uint>, indexBuffer, t, 2, 1);
NRI_RESOURCE(StructuredBuffer<InstanceInfo>, instanceInfoBuffer, t, 3, 1);
NRI_RESOURCE(Texture2D<float4>, sceneTextures[], t, 0, 2);
NRI_RESOURCE(StructuredBuffer<uint>, primitiveIndexBuffers[], t, 0, 3);
NRI_RESOURCE(sampler, Sampler, s, 0, 1);

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

  float2 uvCoord = v0.texcoord * intersect.barycentrics.x +
                   v1.texcoord * intersect.barycentrics.y +
                   v2.texcoord * (1.0f - intersect.barycentrics.x -
                                  intersect.barycentrics.y);

  payload.directLight = float3(float(0.0), float(materialIdx), float(0.0));
  // if (u0 > 100) {
  //   payload.directLight = float3(1.0f, 1.0f, 1.0f);
  // }
}