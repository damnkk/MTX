struct Payload {
  float3 hitValue;
};

struct IntersectionAttributes {
  float2 barycentrics;
};

[shader("closesthit")] void closest_hit(inout Payload payload
                                        : SV_RayPayload,
                                          in IntersectionAttributes intersect
                                        : SV_IntersectionAttributes) {

}