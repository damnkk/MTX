#include "MathLib/ml.hlsli"
#include "NRD/Shaders/Include/NRD.hlsli"
#include "NRICompatibility.hlsli"
#include "RayCommon.hlsli"
NRI_RESOURCE(RWTexture2D<float4>, outputImage, u, 0, 0);
NRI_RESOURCE(RaytracingAccelerationStructure, topLevelAS, t, 1, 0);
NRI_RESOURCE(StructuredBuffer<CameraUniform>, cameraUniform, t, 2, 0);
NRI_ROOT_CONSTANTS(PushConstant, RTConstant, 0,0);

NRI_RESOURCE(StructuredBuffer<MatUniform>, matUniformBuffer, t, 0, 1);
NRI_RESOURCE(StructuredBuffer<Vertex>, vertexBuffer, t, 1, 1);
NRI_RESOURCE(StructuredBuffer<uint>, indexBuffer, t, 2, 1);
NRI_RESOURCE(StructuredBuffer<InstanceInfo>, instanceInfoBuffer, t, 3, 1);
NRI_RESOURCE(SamplerState, Sampler, s, 4, 1);
NRI_RESOURCE(Texture2D<float4>, sceneTextures[], t, 0, 2);
NRI_RESOURCE(Texture2D<float4>, envTextures[], t, 0, 3);
NRI_RESOURCE(StructuredBuffer<uint>, primitiveIndexBuffers[], t, 0, 4);

NRI_RESOURCE(Texture2D<float4>, composed_diff,t,0,5);
NRI_RESOURCE(Texture2D<float4>, composed_spec_viewz,t,1,5);


NRI_RESOURCE(RWTexture2D<float4>,mv_storage,u ,0,5);
NRI_RESOURCE(RWTexture2D<float4>,viewz_storage,u,1,5);
NRI_RESOURCE(RWTexture2D<float4>,normal_roughness_storage,u,2,5);
NRI_RESOURCE(RWTexture2D<float4>,basecolor_metalness_storage,u,3,5);
NRI_RESOURCE(RWTexture2D<float4>,directlight_storage,u,4,5);
NRI_RESOURCE(RWTexture2D<float4>,emission_storage,u,5,5);
NRI_RESOURCE(RWTexture2D<float4>,psr_throughput_storage,u,6,5);            
NRI_RESOURCE(RWTexture2D<float4>,unfiltered_penumbra_storage,u,7,5);
NRI_RESOURCE(RWTexture2D<float4>,unfiltered_translucency_storage,u,8,5);
NRI_RESOURCE(RWTexture2D<float4>,diffuse_storage,u,9,5);
NRI_RESOURCE(RWTexture2D<float4>,specular_storage,u,10,5);   

struct PtPayload
{
  uint   seed;
  float  hitT;
  int    primitiveID;
  int    instanceID;
  int    instanceCustomIndex;
  float2   baryCoord;
  float3x4 objectToWorld;
  float3x4 worldToObject;
};
[shader("raygeneration")] void raygen() {
    Rng::Hash::Initialize(DispatchRaysIndex().xy,RTConstant.curFrameCount);
    float2 pixelOffset = float2(Rng::Hash::GetFloat(),Rng::Hash::GetFloat());
    CameraUniform camUnifor = cameraUniform[0];
    uint2 dispatchRaysIndex = DispatchRaysIndex().xy;
    uint2 dispatchraysDimensions = DispatchRaysDimensions().xy;
    const float2 pixelCenter = float2(dispatchRaysIndex.xy) + pixelOffset;
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
    PtPayload payLoad;
    payLoad.seed = Rng::Hash::GetUint();
    payLoad.hitT = 1e5;
    payLoad.primitiveID = -1;
    payLoad.instanceID = -1;
    payLoad.instanceCustomIndex = -1;
    payLoad.baryCoord = float2(0.0, 0.0);
    TraceRay(topLevelAS,rayFlags,instanceInclusionMask,rayContributionToHitGroupIndex,
    multiplierForGeometryContributionToHitGroupIndex,missShaderIndex,rayDesc,payLoad);

    outputImage[DispatchRaysIndex().xy] = float4(Rng::Hash::GetFloat(),Rng::Hash::GetFloat(),Rng::Hash::GetFloat(),1.0);
    if(payLoad.hitT<1e5){
        outputImage[DispatchRaysIndex().xy] = float4(camUnifor.posFov.xyz,1.0);
    }
}