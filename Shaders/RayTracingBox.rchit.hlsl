#include "BindingBridge.hlsli"
#include "RayCommon.hlsli"
struct InstanceInfo {
  int vertexOffset;
  int indexOffset;
  int vertexCount;
  int indexCount;
  int primitiveInfoIdx;
};

struct MatUniform {
  int test;
};
NRI_RESOURCE(Buffer<MatUniform>, matUniformBuffer, t, 0, 1);
NRI_RESOURCE(Buffer<float>, vertexBuffer, t, 1, 1);
NRI_RESOURCE(Buffer<uint4>, indexBuffer, t, 2, 1);
// NRI_RESOURCE(Buffer<InstanceInfo>, instanceInfoBuffer, t, 3, 1);
NRI_RESOURCE(Buffer<uint>, primitiveIndexBuffers, t, 0, 3);

struct IntersectionAttributes {
  float2 barycentrics;
};

[shader("closesthit")] void closest_hit(inout RayRayloadType payload
                                        : SV_RayPayload,
                                          in IntersectionAttributes intersect
                                        : SV_IntersectionAttributes) {
  payload.directLight = float3(1, 0, 0);
}