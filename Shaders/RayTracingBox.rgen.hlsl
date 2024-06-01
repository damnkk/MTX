#include "BindingBridge.hlsli"
NRI_RESOURCE(RWTexture2D<float4>, outputImage, u, 0, 0);
NRI_RESOURCE(RaytracingAccelerationStructure, topLevelAS, t, 1, 0);

struct Payload {
  float3 hitValue;
};

[shader("raygeneration")] void raygen() {
  uint2 dispatchRaysIndex = DispatchRaysIndex().xy;
  uint2 dispatchraysDimensions = DispatchRaysDimensions().xy;
  const float2 pixelCenter = float2(dispatchRaysIndex.xy) + float2(0.5, 0.5);
  const float2 inUV = pixelCenter / float2(dispatchraysDimensions.xy);
  outputImage[dispatchRaysIndex] = float4(inUV, 0.0, 1.0f);
}
