#include "RtUtils.hlsli"
struct RayRayloadType {
  float4 directLight;
  float3 nextRayOrigin;
  float3 nextRayDirection;
  float3 nextFactor;
  bool shadowRayMiss;
  int level;
};

struct CameraUniform {
  float4x4 ViewToClip;
  float4x4 ClipToView;
  float4x4 WorldToView;
  float4x4 ViewToWorld;
  float4x4 WorldToClip;
  float4x4 ClipToWorld;
  float4 posFov;
  // float2 placeHoader;
};

struct InstanceInfo {
  uint indexOffset;
  uint vertexOffset;
  uint vertexCount;
  uint indexCount;
  uint primitiveInfoIdx;
};

struct MatUniform {
  float4x4 modelMatrix;
  float4 baseColorFactor;
  float3 emissiveFactor;
  float3 envFactor;
  float3 mrFactor;
  float4 intensity;
  int textureUseSetting[4];
  // top5: diffuse, metallicRoughness,ao，normal, emissive
  int textureIndices[32];
};

struct Vertex {
  float3 position;
  float3 normal;
  float4 tangent;
  float2 texcoord;
};

struct PushConstant {
  uint curFrameCount;
  uint maxSampleCount;
  uint maxBounce;
};
