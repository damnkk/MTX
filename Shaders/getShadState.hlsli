#include "RayCommon.hlsli"
#include "layout.hlsli"
struct ShadeState {
  float3 normal;
  float3 geom_normal;
  float3 position;
  float2 text_coords;
  float3 tangent_u;
  float3 tangent_v;
  float3 color;
  uint matIndex;
};
