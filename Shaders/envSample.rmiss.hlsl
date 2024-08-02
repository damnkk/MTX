#include "NRICompatibility.hlsli"
#include "RtUtils.hlsli"

NRI_RESOURCE(Texture2D<float4>, envTextures[], t, 0, 3);
NRI_RESOURCE(SamplerState, Sampler, s, 4, 1);
NRI_RESOURCE(StructuredBuffer<CameraUniform>, cameraUniform, t, 2, 0);

[shader("miss")] void miss(inout envPayload payload
                           : SV_RayPayload) { envPayload.isHit = true; }