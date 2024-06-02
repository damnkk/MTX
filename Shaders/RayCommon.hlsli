
struct RayRayloadType {
  float3 directLight;
  float3 nextRayOrigin;
  float3 nextRayDirection;
  float3 nextFactor;
  bool shadowRayMiss;
  int level;
};

struct CameraUniform {
  float4x4 clipToView;
  float4x4 viewToWorld;
  float4 posFov;
  // float2 placeHoader;
};

struct Test {
  int tt;
  int tt2;
  int tt3;
  int tt4;
  int tt5;
  int tt6;
};