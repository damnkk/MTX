#include "RayCommon.hlsli"
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
[shader("closesthit")] void closest_hit(inout PtPayload payload
                                        : SV_RayPayload,
                                          in IntersectionAttributes intersect
                                        : SV_IntersectionAttributes) {
    payload.hitT = RayTCurrent();
    payload.primitiveID = PrimitiveIndex();
    payload.instanceID = InstanceID();
    payload.instanceCustomIndex = InstanceIndex();
    payload.baryCoord = intersect.barycentrics;
    payload.objectToWorld = ObjectToWorld3x4();
    payload.worldToObject = WorldToObject3x4();



}