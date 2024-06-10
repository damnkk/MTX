#include "NRICompatibility.hlsli"
#include "RayCommon.hlsli"

NRI_RESOURCE(RWTexture2D<float4>, outputImage, u, 0, 0);
NRI_RESOURCE(RaytracingAccelerationStructure, topLevelAS, t, 1, 0);
NRI_RESOURCE(StructuredBuffer<CameraUniform>, cameraUniform, t, 2, 0);

struct Payload {
  float3 hitValue;
};

[shader("raygeneration")] void raygen() {
  // declar
  CameraUniform camUnifor = cameraUniform[0];
  uint2 dispatchRaysIndex = DispatchRaysIndex().xy;
  uint2 dispatchraysDimensions = DispatchRaysDimensions().xy;
  const float2 pixelCenter = float2(dispatchRaysIndex.xy) + float2(0.5, 0.5);
  float2 inUV = pixelCenter / float2(dispatchraysDimensions.xy);
  inUV.y = 1.0f - inUV.y;
  float2 NDC = inUV * 2.0 - 1.0;

  float4 viewCoord =
      mul(camUnifor.clipToView, float4(NDC, camUnifor.posFov.w, 1.0));
  float4 rayDir =
      mul(camUnifor.viewToWorld, float4(normalize(viewCoord.xyz), 0));
  RayDesc rayDesc;
  rayDesc.Origin = float3(camUnifor.posFov.xyz);
  rayDesc.Direction = rayDir.xyz;
  rayDesc.TMin = 0.0001;
  rayDesc.TMax = 100000;

  uint rayFlags = RAY_FLAG_FORCE_OPAQUE;
  uint instanceInclusionMask = 0xff;
  uint rayContributionToHitGroupIndex = 0;
  uint multiplierForGeometryContributionToHitGroupIndex = 1;
  uint missShaderIndex = 0;

  RayRayloadType payLoad;
  payLoad.directLight = float3(0, 0, 0);
  payLoad.nextRayOrigin = float3(0, 0, 0);
  payLoad.nextRayDirection = float3(0, 0, 0);
  payLoad.nextFactor = float3(0, 0, 0);
  payLoad.shadowRayMiss = false;
  payLoad.level = 0;
  TraceRay(topLevelAS, rayFlags, instanceInclusionMask,
           rayContributionToHitGroupIndex,
           multiplierForGeometryContributionToHitGroupIndex, missShaderIndex,
           rayDesc, payLoad);

  outputImage[dispatchRaysIndex] = float4(payLoad.directLight, 1.0f);
  // outputImage[dispatchRaysIndex] = float4(inUV, 0, 1.0f);
}
