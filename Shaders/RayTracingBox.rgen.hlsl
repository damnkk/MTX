#include "BindingBridge.hlsli"
NRI_RESOURCE(RWTexture2D<float4>, outputImage, u, 0, 0);
NRI_RESOURCE(RaytracingAccelerationStructure, topLevelAS, t, 1, 0);

struct Payload {
  float3 hitValue;
};

[shader("raygeneration")] void raygen() {

}
