#include "NRICompatibility.hlsli"
#include "RtUtils.hlsli"

NRI_RESOURCE(Texture2D<float4>, envTextures[], t, 0, 3);
NRI_RESOURCE(SamplerState, Sampler, s, 4, 1);
NRI_RESOURCE(StructuredBuffer<CameraUniform>, cameraUniform, t, 2, 0);

[shader("miss")] void miss(inout RayRayloadType payload
                           : SV_RayPayload) {
  float2 uv = directionToSphericalEnvmap(payload.nextRayDirection);
  Texture2D env = envTextures[0];

  payload.directLight = env.SampleLevel(Sampler, uv, 0.0);
  payload.nextRayOrigin = float3(0.0, 0.0, 0.0);
  payload.nextRayDirection = float3(0.0, 0.0, 0.0);
  payload.level = 1000;
}