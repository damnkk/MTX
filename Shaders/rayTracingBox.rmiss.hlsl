struct Payload {
  float3 hitValue;
};

[shader("miss")] void miss(inout Payload payload
                           : SV_RayPayload) {

}