#define PI 3.14159265358979323
#define TWO_PI 6.28318530717958648
#define INFINITY 1e32
#define EPS 0.0001

static const float M_PI = 3.14159265358979323846;         // pi
static const float M_TWO_PI = 6.28318530717958648;        // 2*pi
static const float M_PI_2 = 1.57079632679489661923;       // pi/2
static const float M_PI_4 = 0.785398163397448309616;      // pi/4
static const float M_1_OVER_PI = 0.318309886183790671538; // 1/pi
static const float M_2_OVER_PI = 0.636619772367581343076; // 2/pi

struct Ray {
  float3 origin;
  float3 direction;
};
struct RayPayloadType {
  uint seed;
  float hitT;
  int primitiveID;
  int instanceID;
  int instanceCustomIndex;
  float2 baryCoord;
  float3x3 objectToWorld;
  float3x3 worldToObject;
};

struct envPayload {
  bool isHit;
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

struct DesnityMaterial {
  float3 albedo;
  float3 emission;
  float3 sheenTint;
  float3 attenuationColor;
  float3 alpha;
  float attenuationDistance;
  float specular;
  float anistropy;
  float metallic;
  float roughness;
  float subsurface;
  float specularTint;
  float sheen;
  float clearcoat;
  float clearcoatRoughness;
  float transmission;
  float ior;

  float ax;
  float ay;
  bool unlit;
  bool tinwalled;
};

struct State {
  int depth;
  float eta;
  float3 position;
  float3 normal;
  float3 ffnormal;
  float3 tangent;
  float3 bitangent;
  float2 texcoord;

  bool isEmitter;
  bool specularBounce;
  bool isSubsurface;

  uint matID;
  DesnityMaterial mat;
};

//-----------------------------------------------------------------------
struct BsdfSampleRec {
  float3 L;
  float3 f;
  float pdf;
};

//-----------------------------------------------------------------------
struct LightSampleRec {
  float3 surfacePos;
  float3 normal;
  float3 emission;
  float pdf;
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

struct IntersectionAttributes {
  float2 barycentrics;
};