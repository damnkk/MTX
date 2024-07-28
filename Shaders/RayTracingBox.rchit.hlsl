#include "NRICompatibility.hlsli"
#include "RtUtils.hlsli"

#include "layout.hlsli"

struct VisibilityContribution {
  float3 radiance; // Radiance at the point if light is visible
  float3 lightDir; // Direction to the light, to shoot shadow ray
  float lightDist; // Distance to the light (1e32 for infinite or sky)
  bool visible;    // true if in front of the face and should shoot shadow ray
};

VisibilityContribution DirectLight(in Ray r, in State state) {
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

  float4 dirPdf; // = EnvSample(lightContrib);
  lightDir = dirPdf.xyz;
  lightPdf = dirPdf.w;

  return contrib;
}

[shader("closesthit")] void closest_hit(inout RayPayloadType payload
                                        : SV_RayPayload,
                                          in IntersectionAttributes intersect
                                        : SV_IntersectionAttributes) {
    payload.hitT = RayTCurrent();
    payload.primitiveID = PrimitiveIndex();
    payload.instanceID = InstanceID();
    payload.instanceCustomIndex = InstanceIndex();
    payload.baryCoord = intersect.barycentrics;
    payload.objectToWorld = (float3x3)ObjectToWorld3x4();
    payload.worldToObject = (float3x3)WorldToObject3x4();
}