#include "BindingBridge.hlsli"
#include "RayCommon.hlsli"

[shader("miss")] void miss(inout RayRayloadType payload
                           : SV_RayPayload) {
  payload.directLight = float3(0, 1, 1);
}