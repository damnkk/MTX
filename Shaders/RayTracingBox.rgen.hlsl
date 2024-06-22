#include "NRICompatibility.hlsli"
#include "RayCommon.hlsli"

NRI_RESOURCE(RWTexture2D<float4>, outputImage, u, 0, 0);
NRI_RESOURCE(RaytracingAccelerationStructure, topLevelAS, t, 1, 0);
NRI_RESOURCE(StructuredBuffer<CameraUniform>, cameraUniform, t, 2, 0);
NRI_PUSH_CONSTANTS(PushConstant, RTConstant, 0);

[shader("raygeneration")] void raygen() {
  // declar
  float2 pixelOffset = hammersley(uint(RTConstant.curFrameCount),
                                  uint(RTConstant.maxSampleCount));
  CameraUniform camUnifor = cameraUniform[0];
  uint2 dispatchRaysIndex = DispatchRaysIndex().xy;
  uint2 dispatchraysDimensions = DispatchRaysDimensions().xy;
  const float2 pixelCenter = float2(dispatchRaysIndex.xy) + pixelOffset;
  // const float2 pixelCenter = float2(dispatchRaysIndex.xy) + float2(0.5, 0.5);
  float2 inUV = pixelCenter / float2(dispatchraysDimensions.xy);
  inUV.y = 1.0f - inUV.y;
  float2 NDC = inUV * 2.0 - 1.0;

  float4 viewCoord =
      mul(camUnifor.ClipToView, float4(NDC, camUnifor.posFov.w, 1.0));
  float4 rayDir =
      mul(camUnifor.ViewToWorld, float4(normalize(viewCoord.xyz), 0));
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
  payLoad.directLight = float4(0, 0, 0, 1.0);
  payLoad.nextRayOrigin = float3(camUnifor.posFov.xyz);
  payLoad.nextRayDirection = rayDir.xyz;
  payLoad.nextFactor = float3(1.0, 1.0, 1.0);
  payLoad.shadowRayMiss = false;
  payLoad.level = 0;
  float3 contribution = float3(1.0f, 1.0f, 1.0f);
  float3 color = float3(0.0, 0.0, 0.0);

  while (length(payLoad.nextRayDirection) > 0.1 &&
         payLoad.level < RTConstant.maxBounce && length(contribution) > 0.001) {
    rayDesc.Origin = payLoad.nextRayOrigin;
    rayDesc.Direction = payLoad.nextRayDirection;
    TraceRay(topLevelAS, rayFlags, instanceInclusionMask,
             rayContributionToHitGroupIndex,
             multiplierForGeometryContributionToHitGroupIndex, missShaderIndex,
             rayDesc, payLoad);
    color += contribution * payLoad.directLight.xyz;
    contribution *= payLoad.nextFactor;
    payLoad.level = payLoad.level + 1;
  }
  if (RTConstant.curFrameCount == 0) {
    outputImage[dispatchRaysIndex] = float4(color, 1.0f);
  } else {
    float3 previousAverage = outputImage[dispatchRaysIndex].rgb;
    // previousAverage = pow(previousAverage, float3(2.2, 2.2, 2.2));
    float3 newAverage =
        (previousAverage.rgb * float(RTConstant.curFrameCount) + color) /
        float(RTConstant.curFrameCount + 1);

    // newAverage = pow(newAverage, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    outputImage[dispatchRaysIndex] = float4(newAverage, 1.0f);
  }

  // outputImage[dispatchRaysIndex] =
  //     float4(RTConstant.curFrameCount, 0.0, 0.0, 1.0f);
}
