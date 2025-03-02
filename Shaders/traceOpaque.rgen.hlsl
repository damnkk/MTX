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
NRI_RESOURCE(RWTexture2D<float>,viewz_storage,u,1,5);
NRI_RESOURCE(RWTexture2D<float4>,normal_roughness_storage,u,2,5);
NRI_RESOURCE(RWTexture2D<float4>,basecolor_metalness_storage,u,3,5);
NRI_RESOURCE(RWTexture2D<float3>,directlight_storage,u,4,5);
NRI_RESOURCE(RWTexture2D<float3>,emission_storage,u,5,5);
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

struct GeometryProps
{
  float3 X;
  float3 Xprev;
  float3 N;
  float3 V;
  float4 T;
  float2 uv;
  float hitT;
  int mip;
  uint instanceIndex;
  uint materialIndex;
};

struct MaterialProps
{
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
  float3 Ldirect;

  float ax;
  float ay;
  bool unlit;
  bool tinwalled;
};

float2 GetConeAngleFromAngularRadius( float mip, float tanConeAngle )
{
    // In any case, we are limited by the output resolution
    tanConeAngle = max( tanConeAngle, cameraUniform[0].tanPixelAngularRadius );

    return float2( mip, tanConeAngle );
}

float2 GetConeAngleFromRoughness( float mip, float roughness )
{
    float tanConeAngle = roughness * roughness * 0.05; // TODO: tweaked to be accurate and give perf boost

    return GetConeAngleFromAngularRadius( mip, tanConeAngle );
}

#define SKY_INTENSITY 1.0
#define SUN_INTENSITY 10.0
float3 GetSunIntensity(float3 v){
  float b = dot(v,cameraUniform[0].sunDirection.xyz);
  float d = length(v-cameraUniform[0].sunDirection.xyz*b);
  float glow = saturate(1.015-d);
  glow *= b*0.5+0.5;
  glow *= 0.6;
  float a = Math::Sqrt01(1.0-b*b)/b;
  float sun = 1.0-Math::SmoothStep(cameraUniform[0].tanSunAngularRadius*0.9,cameraUniform[0].tanSunAngularRadius*0.9* 1.66 + 0.01, a);
  sun *= float(b>0.0);
  sun *= 1.0-Math::Pow01(1.0-v.z,4.85);
  sun *= Math::SmoothStep(0.0,0.1,cameraUniform[0].sunDirection.z);
  sun += glow;
  float sunColor = lerp( float3( 1.0, 0.6, 0.3 ), float3( 1.0, 0.9, 0.7 ), Math::Sqrt01( cameraUniform[0].sunDirection.z ) );
  sunColor *= sun;
  sunColor *= Math::SmoothStep(-0.01,0.05,cameraUniform[0].sunDirection.z);
  
  return Color::FromGamma(sunColor)*SUN_INTENSITY;
}

float3 GetSkyIntensity( float3 v )
{
    float atmosphere = sqrt( 1.0 - saturate( v.z ) );

    float scatter = pow( saturate( cameraUniform[0].sunDirection.z ), 1.0 / 15.0 );
    scatter = 1.0 - clamp( scatter, 0.8, 1.0 );

    float3 scatterColor = lerp( float3( 1.0, 1.0, 1.0 ), float3( 1.0, 0.3, 0.0 ) * 1.5, scatter );
    float3 skyColor = lerp( float3( 0.2, 0.4, 0.8 ), float3( scatterColor ), atmosphere / 1.3 );
    skyColor *= saturate( 1.0 + cameraUniform[0].sunDirection.z );

    float ground = 0.5 + 0.5 * Math::SmoothStep( -1.0, 0.0, v.z );
    skyColor *= ground;

    return Color::FromGamma( skyColor ) * SKY_INTENSITY + GetSunIntensity( v );
}


float3 GetMotion( float3 X, float3 Xprev )
{
    CameraUniform camUnifor = cameraUniform[0];
    float3 motion = Xprev - X;

    float viewZ = Geometry::AffineTransform( camUnifor.WorldToView, X ).z;
    float2 sampleUv = Geometry::GetScreenUv( camUnifor.WorldToClip, X );

    float viewZprev = Geometry::AffineTransform( camUnifor.WorldToViewPrev, Xprev ).z;
    float2 sampleUvPrev = Geometry::GetScreenUv( camUnifor.WorldToClipPrev, Xprev );

    // // IMPORTANT: scaling to "pixel" unit significantly improves utilization of FP16
    motion.xy = ( sampleUvPrev - sampleUv ) * float2(float(DispatchRaysDimensions().x),float(DispatchRaysDimensions().y));

    // // IMPORTANT: 2.5D motion is preferred over 3D motion due to imprecision issues caused by FP16 rounding negative effects
    motion.z = viewZprev - viewZ;

    return motion;
}

float3 normalMap(float3 vertexNormal, float3 tagNormal) {
  float3 tagent = normalize(cross(vertexNormal, float3(1.0f, 0.0f, 0.0f)));
  float3 bitTagent = normalize(cross(vertexNormal, tagent));
  return normalize(tagent * tagNormal.x + bitTagent * tagNormal.y +
                   vertexNormal * tagNormal.z);
}

// GeometryProps CaseRay(float origin, float3 direction, float Tmin, float Tmax, float2 mipAndCone,RaytracingAccelerationStructure accelerationStructure,  uint instanceInclusionMask, uint rayFlags){
//   GeometryProps res = ( GeometryProps )0;
//   res.mip = mipAndCone.x;
//   RayDesc rayDesc;
//   rayDesc.Origin = origin;
//   rayDesc.Direction = direction.xyz;
//   rayDesc.TMin = 0.0001;
//   rayDesc.TMax = 100000;

//   uint rayFlags = RAY_FLAG_FORCE_OPAQUE;
//   uint instanceInclusionMask = 0xff;
//   uint rayContributionToHitGroupIndex = 0;
//   uint multiplierForGeometryContributionToHitGroupIndex = 1;
//   uint missShaderIndex = 0;

//   PtPayload payLoad;
//   payLoad.seed = Rng::Hash::GetUint();
//   payLoad.hitT = 1e5;
//   payLoad.primitiveID = -1;
//   payLoad.instanceID = -1;
//   payLoad.instanceCustomIndex = -1;
//   payLoad.baryCoord = float2(0.0, 0.0);
//   TraceRay(topLevelAS,rayFlags,instanceInclusionMask,rayContributionToHitGroupIndex,
//   multiplierForGeometryContributionToHitGroupIndex,missShaderIndex,rayDesc,payLoad);
//    if(payLoad.hitT==1e5){
//       res.hitT = 1e5;
//       res.X = rayDesc.Origin + rayDesc.Direction * res.hitT;
//       res.Xprev = res.X;
//       res.V = -rayDesc.Direction;
//    }else{
//       res.hitT = payLoad.hitT;
//       uint instanceIndex = payLoad.instanceID;
//       res.instanceIndex = instanceIndex;
//       InstanceInfo instaInfo = instanceInfoBuffer[payLoad.instanceID];
//       float3x3 mObjectToWorld = ( float3x3 )payLoad.objectToWorld;
//       float3 barycentrics = float3(1.0 - payLoad.baryCoord.x - payLoad.baryCoord.y,payLoad.baryCoord.xy);
//       uint u0 = indexBuffer[instaInfo.indexOffset + 3 * payLoad.primitiveID + 0];
//       uint u1 = indexBuffer[instaInfo.indexOffset + 3 * payLoad.primitiveID + 1];
//       uint u2 = indexBuffer[instaInfo.indexOffset + 3 * payLoad.primitiveID + 2];

//       Vertex v0 = vertexBuffer[instaInfo.vertexOffset + u0];
//       Vertex v1 = vertexBuffer[instaInfo.vertexOffset + u1];
//       Vertex v2 = vertexBuffer[instaInfo.vertexOffset + u2];
//       //normal
//       float3  vertNormal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
//       float flip = dot(normalize(vertNormal), rayDesc.Direction)>0?-1.0:1.0;
//       vertNormal = Geometry::RotateVector((float3x3)payLoad.objectToWorld, vertNormal);
//       vertNormal = normalize(vertNormal*flip);
//       res.N = -vertNormal;

//       res.uv = v0.texcoord * barycentrics.x + v1.texcoord * barycentrics.y +
//                 v2.texcoord * barycentrics.z;

//       float3 T = v0.tangent.xyz * barycentrics.x + v1.tangent.xyz * barycentrics.y + v2.tangent.xyz * barycentrics.z; 
//       T = normalize(Geometry::RotateVector((float3x3)payLoad.objectToWorld, T));
//       res.T = float4(T, 1.0);
//       res.X = origin + direction * res.hitT;
//       res.Xprev = res.X;
//       res.V = -rayDesc.Direction;
//    }
//   return res;
// }

MaterialProps GetMaterialProps(GeometryProps geometryProps, bool viewIndependentLightingModel = false ){
    MaterialProps matProps = ( MaterialProps )0;
    return matProps;
}



[shader("raygeneration")] void raygen() {
    Rng::Hash::Initialize(DispatchRaysIndex().xy,RTConstant.curFrameCount);
    float2 pixelOffset = float2(Rng::Hash::GetFloat(),Rng::Hash::GetFloat());
    CameraUniform camUnifor = cameraUniform[0];
    uint2 dispatchRaysIndex = DispatchRaysIndex().xy;
    uint2 dispatchraysDimensions = DispatchRaysDimensions().xy;
    const float2 pixelCenter = float2(dispatchRaysIndex.xy);
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
    GeometryProps geoProps;
    MaterialProps matProps;
    { 
      if(payLoad.hitT==1e5){
        geoProps.hitT = 1e5;
        geoProps.X = rayDesc.Origin + rayDesc.Direction * geoProps.hitT;
        geoProps.Xprev = geoProps.X;
        geoProps.V = -rayDesc.Direction;
      }
      else{
        InstanceInfo instaInfo = instanceInfoBuffer[payLoad.instanceID];
        uint u0 = indexBuffer[instaInfo.indexOffset + 3 * payLoad.primitiveID + 0];
        uint u1 = indexBuffer[instaInfo.indexOffset + 3 * payLoad.primitiveID + 1];
        uint u2 = indexBuffer[instaInfo.indexOffset + 3 * payLoad.primitiveID + 2];

        Vertex v0 = vertexBuffer[instaInfo.vertexOffset + u0];
        Vertex v1 = vertexBuffer[instaInfo.vertexOffset + u1];
        Vertex v2 = vertexBuffer[instaInfo.vertexOffset + u2];

        float3 baryCentrics =float3(1.0 - payLoad.baryCoord.x - payLoad.baryCoord.y,
             payLoad.baryCoord.x, payLoad.baryCoord.y);
        float2 uvCoord = v0.texcoord * baryCentrics.x + v1.texcoord * baryCentrics.y +
                v2.texcoord * baryCentrics.z;
        float3 vertPosition = v0.position * baryCentrics.x + v1.position * baryCentrics.y + v2.position * baryCentrics.z;
        vertPosition = Geometry::RotateVector((float3x3)payLoad.objectToWorld,vertPosition);
        float3  vertNormal = v0.normal * baryCentrics.x + v1.normal * baryCentrics.y + v2.normal * baryCentrics.z;
        vertNormal = Geometry::RotateVector((float3x3)payLoad.objectToWorld, vertNormal);
        float3 vertTagent = normalize(v0.tangent.xyz * baryCentrics.x + v1.tangent.xyz * baryCentrics.y + v2.tangent.xyz * baryCentrics.z);
        vertTagent = Geometry::RotateVector((float3x3)payLoad.objectToWorld,vertTagent);
        uint materialIdx = primitiveIndexBuffers[instaInfo.primitiveInfoIdx][payLoad.primitiveID];
        MatUniform mat = matUniformBuffer[materialIdx];

        float3 baseColor = float3(0.5, 0.5, 0.5);
        if (mat.textureIndices[0] > -1) {
          Texture2D baseColorTexture = sceneTextures[mat.textureIndices[0]];
          baseColor = baseColorTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
        }

        float3 metallicRoughness = float3(0.0, 0.0, 0.0);
        if (mat.textureIndices[1] > -1) {
          Texture2D mrTexture = sceneTextures[mat.textureIndices[1]];
          metallicRoughness = mrTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
        }

        float ao = 0.0f;
        if (mat.textureIndices[2] > -1) {
          Texture2D aoTexture = sceneTextures[mat.textureIndices[2]];
          ao = aoTexture.SampleLevel(Sampler, uvCoord, 0.0).x;
        }

        float3 normal = normalize(vertNormal);
        if (mat.textureIndices[3] > -1) {
          Texture2D normalTexture = sceneTextures[mat.textureIndices[3]];
          float3 tagNormal = normalTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz*2.0-1.0;
          normal = normalMap(vertNormal, tagNormal);
        }

        float3 emissive = float3(0.0f, 0.0f, 0.0f);
        if (mat.textureIndices[4] > -1) {
          Texture2D emissiveTexture = sceneTextures[mat.textureIndices[4]];
          emissive = emissiveTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
        }
        //-----------------material properties------------------------------
        matProps.albedo = baseColor;
        matProps.emission = emissive;
        matProps.sheenTint = float3(0.0f, 0.0f, 0.0f);
        matProps.attenuationColor = float3(1.0f, 1.0f, 1.0f);
        matProps.alpha = float3(1.0f, 1.0f, 1.0f);
        matProps.attenuationDistance = 1.0f;
        matProps.specular = metallicRoughness.x;
        matProps.anistropy = 0.0;
        matProps.metallic = metallicRoughness.z;
        matProps.roughness = metallicRoughness.y;
        matProps.subsurface = 0.0f;
        matProps.specularTint = 0.0f;
        matProps.sheen = 0.0f;
        matProps.clearcoat = 0.0f;
        matProps.clearcoatRoughness = 0.0f;
        matProps.transmission = 0.0f;
        matProps.ior = 1.0f;
        float aspect = sqrt(1.0 - matProps.anistropy * 0.9);
        matProps.ax = max(0.001, matProps.roughness / aspect);
        matProps.ay = max(0.001, matProps.roughness * aspect);

        geoProps.hitT = payLoad.hitT;
        geoProps.instanceIndex = payLoad.instanceID;
        if(dot(rayDesc.Direction,normal)>0.0){
          normal = -normal;
        }
        geoProps.N = normal;
        geoProps.uv = uvCoord;
        geoProps.T =float4(vertTagent,1.0);
        geoProps.X = vertPosition;
        // we update the previous position to the current position,cuz our scene is static currently,
        // in the future, if we want play with a dynamic scene(move object or deform object), we need 
        // to stage the last 3 matrixies for calculating the motion information.
        geoProps.Xprev = geoProps.X;
        geoProps.V = -rayDesc.Direction;

        float2 mipAndCone = GetConeAngleFromRoughness(0.0,0.0);
        geoProps.mip = mipAndCone.x;
        float NoRay = abs(dot(rayDir.xyz,vertNormal));
        float a = payLoad.hitT *mipAndCone.y;
        a *= Math::PositiveRcp(NoRay);
        float mip = log2(a);
        mip = max(mip,0.0);
        geoProps.mip = mip;

        float3 Ldirect = 0;
        float NoL = saturate(dot(geoProps.N,cameraUniform[0].sunDirection.xyz));
        float shadow =  Math::SmoothStep(0.03,0.1,NoL);
        [branch]
        if(shadow != 0.0){
          float3 Csun = GetSunIntensity(cameraUniform[0].sunDirection.xyz);
          float3 Csky = GetSkyIntensity(-geoProps.V);
          bool hasHair = false;
          if(hasHair){}
          else{
            float3 Cimp = lerp(Csky,Csun,Math::SmoothStep(0.0,0.2,matProps.roughness));
            Cimp *= Math::SmoothStep(-0.01,0.05,cameraUniform[0].sunDirection.z);

            float3 albedo,Rf0;
            BRDF::ConvertBaseColorMetalnessToAlbedoRf0( matProps.albedo.xyz, matProps.metallic, albedo, Rf0 );

            float3 Cdiff,Cspec;
            BRDF::DirectLighting( geoProps.N, cameraUniform[0].sunDirection.xyz, geoProps.V, Rf0, matProps.roughness, Cdiff, Cspec );
            Ldirect = Cdiff *albedo*Csun+Cspec*Cimp;
            // Ldirect = Rf0;
          }
        }
        Ldirect  *= shadow;
        matProps.Ldirect = Ldirect;
      }
    }

    //write viewZ to storage
    float3 hitPos = rayDesc.Origin + rayDesc.Direction * payLoad.hitT;
    float viewZ = Geometry::AffineTransform(camUnifor.WorldToView,float4(hitPos,1.0)).z;
    viewZ = payLoad.hitT == 1e5?  Math::Sign( viewZ ) * 1e5 : viewZ;
    viewz_storage[DispatchRaysIndex().xy]= viewZ;

    //write mv to storage
    float3 motion = GetMotion(geoProps.X,geoProps.Xprev);
    float viewZAndTaaMask = abs(viewZ)*0.125;
    mv_storage[DispatchRaysIndex().xy] = float4(motion,viewZAndTaaMask);
      if(payLoad.hitT==1e5){
      emission_storage[DispatchRaysIndex().xy] = float3(matProps.emission.xyz);
      return;
    }

   //G-buffer
    uint materialID = matProps.metallic<0.5? MATERIAL_ID_DEFAULT : MATERIAL_ID_METAL;
    normal_roughness_storage[DispatchRaysIndex().xy] = NRD_FrontEnd_PackNormalAndRoughness(geoProps.N, matProps.roughness,materialID);
    basecolor_metalness_storage[DispatchRaysIndex().xy] = float4(Color::ToSrgb(matProps.albedo), matProps.metallic);
    directlight_storage[DispatchRaysIndex().xy] = matProps.Ldirect;
    emission_storage[DispatchRaysIndex().xy] = matProps.emission;

    outputImage[DispatchRaysIndex().xy] = float4(Rng::Hash::GetFloat(),Rng::Hash::GetFloat(),Rng::Hash::GetFloat(),1.0);
    if(payLoad.hitT<1e5){
        outputImage[DispatchRaysIndex().xy] =  float4(float3(matProps.Ldirect),1.0);
    }
}