#include "NRICompatibility.hlsli"
#include "RtUtils.hlsli"
#include "layout.hlsli"

float3 PathTrace(float3 _rayOrigin,float3 _rayDirection,inout RayPayloadType prd){
  float3 radiance = float3(0.0,0.0,0.0);
  float3 throughput = float3(1.0,1.0,1.0);
  float3 absorption = float3(0.0,0.0,0.0);

  float3 rayOrigin = _rayOrigin;
  float3 rayDirection = _rayDirection;
  for(int depth = 0;depth <RTConstant.maxBounce;depth++){
    ClosestHit(rayOrigin,rayDirection,prd,topLevelAS);
    if(prd.hitT == INFINITY){
      float3 env;
      float2 uv = directionToSphericalEnvmap(rayDirection);
      Texture2D envSample = envTextures[0];
      env = envSample.SampleLevel(Sampler, uv, 0.0).xyz;
      return radiance+(env*throughput);
    }
    uint instanceID = prd.instanceID;
    uint primitiveIndex = prd.primitiveID;
    InstanceInfo instaInfo = instanceInfoBuffer[instanceID];
    uint u0 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 0];
    uint u1 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 1];
    uint u2 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 2];

    Vertex v0 = vertexBuffer[instaInfo.vertexOffset + u0];
    Vertex v1 = vertexBuffer[instaInfo.vertexOffset + u1];
    Vertex v2 = vertexBuffer[instaInfo.vertexOffset + u2];

    CameraUniform camUnifor = cameraUniform[0];
    float3 baryCentrics =float3(1.0 - prd.baryCoord.x - prd.baryCoord.y,
             prd.baryCoord.x,prd.baryCoord.y);
    float2 uvCoord = v0.texcoord * baryCentrics.x + v1.texcoord * baryCentrics.y +
                   v2.texcoord * baryCentrics.z;
    float3 vertPosition = v0.position * baryCentrics.x +
                        v1.position * baryCentrics.y +
                        v2.position * baryCentrics.z;
    vertPosition = mul( prd.objectToWorld, vertPosition).xyz;
    float3 vertNormal = v0.normal * baryCentrics.x + v1.normal * baryCentrics.y +
                        v2.normal * baryCentrics.z;
    vertNormal = mul( prd.objectToWorld, vertNormal).xyz;

    float3 vertTagent = normalize(v0.tangent.xyz * baryCentrics.x +
                                  v1.tangent.xyz * baryCentrics.y +
                                  v2.tangent.xyz * baryCentrics.z);
    vertTagent = normalize(mul( prd.objectToWorld, vertTagent).xyz);
    uint materialIdx =
      primitiveIndexBuffers[instaInfo.primitiveInfoIdx][primitiveIndex];
    MatUniform mat = matUniformBuffer[materialIdx];
    float3 baseColor = float3(0.5, 0.5, 0.5);
    if (mat.textureIndices[0] > -1) {
      Texture2D baseColorTexture = sceneTextures[mat.textureIndices[0]];
      baseColor = SRGBtoLINEAR(baseColorTexture.SampleLevel(Sampler, uvCoord, 0.0)).xyz;
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
      float3 tagNormal = normalTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
      normal = normalMap(vertNormal, tagNormal,vertTagent);
    }

    float3 emissive = float3(0.0f, 0.0f, 0.0f);
    if (mat.textureIndices[4] > -1) {
      Texture2D emissiveTexture = sceneTextures[mat.textureIndices[4]];
      emissive = emissiveTexture.SampleLevel(Sampler, uvCoord, 0.0).xyz;
    }
      //--------------------------------------------------------------------------

    float3 bitTangent = normalize(cross(vertTagent, normal));
    State state;
    state.position = vertPosition;
    state.normal = normal;
    state.tangent = vertTagent;
    state.bitangent = bitTangent;
    state.isEmitter = false;
    state.specularBounce = false;
    state.isSubsurface = false;
    state.ffnormal = dot(state.normal, rayDirection) <= 0.0
                        ? state.normal
                        : -state.normal;
    CreateCoordinateSystem(state.ffnormal, state.tangent, state.bitangent);
    state.mat.specular = 0.5;
    state.mat.subsurface = 0.0;
    state.mat.specularTint = 1.0;
    state.mat.sheen = 0.0;
    state.mat.sheenTint = float3(0.0, 0.0, 0.0);
    state.texcoord = uvCoord;
    state.mat.albedo = baseColor;
    state.mat.emission = emissive;
    state.mat.roughness = max(metallicRoughness.y, 0.001);
    // state.mat.roughness = 0.001;
    state.mat.metallic = max(metallicRoughness.z, 0.001);
    // state.mat.metallic = 0.00001;
    state.mat.transmission = mat.intensity.z;
    state.mat.transmission = 0.0;
    state.mat.ior = 1.33;
    state.eta = dot(state.normal, state.ffnormal) > 0.0 ? (1.0 / state.mat.ior)
                                                        : state.mat.ior;
    state.mat.anistropy = 0.0;
    float aspect = sqrt(1.0 - state.mat.anistropy * 0.9);
    state.mat.ax = 0.000;
    state.mat.ay = 0.000;

    state.mat.clearcoat = 0.0f;
    state.mat.clearcoatRoughness = 0.001;

    state.mat.sheenTint = float3(0.0, 0.0, 0.0);
    state.mat.sheen = 0.0;

    BsdfSampleRec bsdfSampleRec;
    
    // return float3(randomNum,0.0f);
    bsdfSampleRec.f = DisneySample(state, rayDirection, state.ffnormal, bsdfSampleRec.L, bsdfSampleRec.pdf, prd.seed);
    
    if(bsdfSampleRec.pdf > 0.0)
    {
      throughput *= bsdfSampleRec.f * abs(dot(state.ffnormal, bsdfSampleRec.L)) / bsdfSampleRec.pdf;
    }
    else
    {
      break;
    }
    rayDirection = bsdfSampleRec.L;
    rayOrigin = OffsetRay(vertPosition, dot(bsdfSampleRec.L, state.ffnormal) > 0 ? state.ffnormal : -state.ffnormal);
  }
  return radiance;
}

float3 samplePixel(int2 imageCoords,int2 sizeImage,RayPayloadType prd){
  float2 subpixel_jitter = RTConstant.curFrameCount == 0? float2(0.5f,0.5f):float2(rand(prd.seed),rand(prd.seed));
  const float2 pixelCenter = float2(imageCoords)+subpixel_jitter;
  float2 inUV =pixelCenter/float2(sizeImage);
  inUV.y = 1.0f-inUV.y;
  float2 d = inUV*2.0-1.0;
  
  CameraUniform camUnifor = cameraUniform[0];
  float4 origin =camUnifor.posFov;
  float4 target =  mul(camUnifor.ClipToView,float4(d.x,d.y,1.0,1.0));
  float4 direction =  mul(camUnifor.ViewToWorld,float4(normalize(target.xyz),0.0));
  
  float3 radiance = PathTrace(origin.xyz,direction.xyz,prd);

  return radiance;
}

[shader("raygeneration")] void raygen() {

  int2 imageCoords =int2( DispatchRaysIndex().xy);
  int2 imageRes = int2(DispatchRaysDimensions().xy);

  RayPayloadType payLoad;
  payLoad.seed = initRandom(DispatchRaysDimensions().xy, DispatchRaysIndex().xy,
                         RTConstant.curFrameCount);
  float3 pixelColor = float3(0.0,0.0,0.0);
  int maxSampleCount = 1;
  for(int smpl = 0;smpl<maxSampleCount;++smpl){
    pixelColor +=samplePixel(imageCoords,imageRes,payLoad);
  }
  pixelColor /=maxSampleCount;


  if(RTConstant.curFrameCount>0){
    float3 old_color =  outputImage[imageCoords].rgb;
    float3 new_result = lerp(old_color,pixelColor,1.0f/(float(RTConstant.curFrameCount)+1.0f));
    // new_result.x = new_result.x*(1.0f-1.0f/(asfloat(RTConstant.curFrameCount)*1.0))+pixelColor.x,1.0f/(asfloat(RTConstant.curFrameCount)*1.0);
    outputImage[imageCoords] = float4(new_result,1.0f);
  }else if(RTConstant.curFrameCount==0){
    outputImage[imageCoords] = float4(pixelColor,1.0f);
  }
}
