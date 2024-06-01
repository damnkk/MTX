#include "BindingBridge.hlsli"
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

struct Payload {
  float3 hitValue;
};

struct IntersectionAttributes {
  float2 barycentrics;
};

[shader("closesthit")] void closest_hit(inout Payload payload
                                        : SV_RayPayload,
                                          in IntersectionAttributes intersect
                                        : SV_IntersectionAttributes) {
  float test = vertexBuffer[3];
  payload.hitValue = float3(test, test, test);
}