#include "NRICompatibility.hlsli"
#include "RtUtils.hlsli"

#include "layout.hlsli"

struct VisibilityContribution {
  float3 radiance; // Radiance at the point if light is visible
  float3 lightDir; // Direction to the light, to shoot shadow ray
  float lightDist; // Distance to the light (1e32 for infinite or sky)
  bool visible;    // true if in front of the face and should shoot shadow ray
};

float powerHeuristic(float a, float b)
//-----------------------------------------------------------------------
{
  float t = a * a;
  return t / (b * b + t);
}

float3 Environment_sample(Texture2D<float4> envMap,in float3 randVal,out float3 to_light,out float pdf){
  float3 xi = randVal;
  //HLSL buildin function to get the Texture2D size
  uint mipmapLevel,width,height;
  envMap.GetDimensions(width,height);
  const uint size = width*height;
  const uint idx = min(uint(xi.x*float(size)),size-1);
  EnvAccel sample_data = EnvPdfBuffer[idx];
  uint env_idx;
  if(xi.y<sample_data.q){
    env_idx = idx;
    xi.y/=sample_data.q;
    pdf = sample_data.pdf;
  }else{
    env_idx = sample_data.alias;
    xi.y = (xi.y-sample_data.q)/(1.0f-sample_data.q);
    pdf = sample_data.aliasPdf;
  }

  const uint px = env_idx%width;
  uint py = env_idx/width;

  const float u = float(px+xi.y)/float(width);
  const float phi = u*(2.0f*M_PI) - M_PI;
  float sin_phi = sin(phi);
  float cos_phi = cos(phi);

  const float step_theta = M_PI/float(height);
  const float theta0     = float(py) * step_theta;
  const float cos_theta  = cos(theta0) * (1.0f - xi.z) + cos(theta0 + step_theta) * xi.z;
  const float theta      = acos(cos_theta);
  const float sin_theta  = sin(theta);
  const float v          = theta * M_1_OVER_PI;

  to_light = float3(cos_phi*sin_theta,cos_theta,sin_phi*sin_theta);

  return envMap.SampleLevel(Sampler,float2(u,v),0.0).xyz;
}

float4 EnvSample(inout float3 radiance,inout RayRayloadType payLoad){
  float3 lightDir=float3(0.0,1.0,0.0);
  float pdf;
  float3 randVal = float3(rand(payLoad.seed),rand(payLoad.seed),rand(payLoad.seed));
  radiance  = Environment_sample(envTextures[0],randVal,lightDir,pdf);
  return float4(lightDir,pdf);
}

VisibilityContribution DirectLight(in Ray r, in State state,inout RayRayloadType payLoad) {
  float3 Li = float3(0.0, 0.0, 0.0);
  float lightPdf;
  float3 lightContrib;
  float3 lightDir;
  float lightDist = 1e32;
  bool isLight = false;

  VisibilityContribution contrib;
  contrib.radiance = float3(0.0, 0.0, 0.0);
  contrib.visible = false;

  // sample Env
  float4 dirPdf = EnvSample(lightContrib,payLoad);
  lightDir = dirPdf.xyz;
  lightPdf = dirPdf.w;
  if(dot(lightDir,state.ffnormal)>0.0){
    {
      BsdfSampleRec bsdfSampleRec={float3(0.0,0.0,0.0),float3(0.0,0.0,0.0),0.0};
      bsdfSampleRec.f = DisneyEval(state,-r.direction,state.ffnormal,lightDir,bsdfSampleRec.pdf);
      float misWeight=  max(0.0,powerHeuristic(lightPdf,bsdfSampleRec.pdf));
      Li += misWeight*bsdfSampleRec.f * abs(dot(lightDir,state.ffnormal))*lightContrib/lightPdf;
    }
    contrib.visible = true;
    contrib.lightDir = lightDir;
    contrib.lightDist = lightDist;
    contrib.radiance = Li;
  }
  return contrib;
}

[shader("closesthit")] void closest_hit(inout RayRayloadType payload
                                        : SV_RayPayload,
                                          in IntersectionAttributes intersect
                                        : SV_IntersectionAttributes) {
  // 得到实例索引
  uint instanceID = InstanceID();
  // 得到图元ID
  uint primitiveIndex = PrimitiveIndex();
  // 根据实例索引得到该实例的几何索引信息
  InstanceInfo instaInfo = instanceInfoBuffer[instanceID];

  uint u0 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 0];
  uint u1 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 1];
  uint u2 = indexBuffer[instaInfo.indexOffset + 3 * primitiveIndex + 2];

  Vertex v0 = vertexBuffer[instaInfo.vertexOffset + u0];
  Vertex v1 = vertexBuffer[instaInfo.vertexOffset + u1];
  Vertex v2 = vertexBuffer[instaInfo.vertexOffset + u2];

  CameraUniform camUnifor = cameraUniform[0];

  float3 baryCentrics =
      float3(1.0 - intersect.barycentrics.x - intersect.barycentrics.y,
             intersect.barycentrics.x, intersect.barycentrics.y);

  float2 uvCoord = v0.texcoord * baryCentrics.x + v1.texcoord * baryCentrics.y +
                   v2.texcoord * baryCentrics.z;
  float3 vertPosition = v0.position * baryCentrics.x +
                        v1.position * baryCentrics.y +
                        v2.position * baryCentrics.z;
  vertPosition = mul((float3x3)ObjectToWorld3x4(), vertPosition).xyz;
  float3 vertNormal = v0.normal * baryCentrics.x + v1.normal * baryCentrics.y +
                      v2.normal * baryCentrics.z;
  vertNormal = mul((float3x3)ObjectToWorld3x4(), vertNormal).xyz;

  float3 vertTagent = normalize(v0.tangent.xyz * baryCentrics.x +
                                v1.tangent.xyz * baryCentrics.y +
                                v2.tangent.xyz * baryCentrics.z);
  vertTagent = normalize(mul((float3x3)ObjectToWorld3x4(), vertTagent).xyz);

  // 对应的那个实例的primitiveMaterialBuffer，之后根据primitiveID拿到材质索引
  uint materialIdx =
      primitiveIndexBuffers[instaInfo.primitiveInfoIdx][primitiveIndex];
  // 拿到材质
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
  state.ffnormal = dot(state.normal, payload.nextRayDirection) <= 0.0
                       ? state.normal
                       : -state.normal;
  CreateCoordinateSystem(state.ffnormal, state.tangent, state.bitangent);
  state.mat.specular = 1.0;
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
  state.mat.ax = max(0.001, state.mat.roughness / aspect);
  state.mat.ay = max(0.001, state.mat.roughness * aspect);

  state.mat.clearcoat = 0.0f;
  state.mat.clearcoatRoughness = 0.001;

  state.mat.sheenTint = float3(0.0, 0.0, 0.0);
  state.mat.sheen = 0.0;
  // add emissive light
  payload.directLight = float4(emissive, 1.0);

  // direct light sampling
  Ray r;
  r.origin = payload.nextRayOrigin;
  r.direction = payload.nextRayDirection;
  VisibilityContribution vcontrib = DirectLight(r,state,payload);
  // sampleEnv
  BsdfSampleRec bsdfSampleRec;
  bsdfSampleRec.L = float3(0.0, 0.0, 0.0);
  bsdfSampleRec.pdf = 0.0f;

  bsdfSampleRec.f =
      DisneySample(state, -payload.nextRayDirection, state.ffnormal,
                   bsdfSampleRec.L, bsdfSampleRec.pdf, payload.seed);

  if (bsdfSampleRec.pdf > 0.0) {
    payload.nextFactor = bsdfSampleRec.f *
                         abs(dot(state.ffnormal, bsdfSampleRec.L)) /
                         bsdfSampleRec.pdf;
  } else {
    payload.level = 100;
    return;
  }

  payload.nextRayDirection = bsdfSampleRec.L;
  payload.nextRayOrigin = OffsetRay(
      vertPosition, dot(bsdfSampleRec.L, state.ffnormal) > 0 ? state.ffnormal
                                                             : -state.ffnormal);
  if(vcontrib.visible == true){
    // float3 shadowRayDirection = vcontrib.lightDir;
    float3 shadowRayDirection = payload.nextRayDirection;
    float3 shadowRayOrigin = payload.nextRayOrigin;
    RayDesc rayDesc;
    rayDesc.Origin = shadowRayOrigin;
    rayDesc.Direction = shadowRayDirection;
    rayDesc.TMin = 0.0001;
    rayDesc.TMax = 100000;

    uint rayFlags = RAY_FLAG_FORCE_OPAQUE|RAY_FLAG_SKIP_CLOSEST_HIT_SHADER|RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
    EnvPayload shadowPLoad;
    TraceRay(topLevelAS,rayFlags, 0xff,0,1,1,rayDesc,shadowPLoad);
    if(!shadowPLoad.isHit){
      payload.directLight.xyz +=vcontrib.radiance;
    }
  }
}