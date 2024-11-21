#include "NRICompatibility.hlsli"
NRI_RESOURCE(RWTexture2D<float4>, outputImage, u, 0, 0);

[shader("raygeneration")] void raygen() {
    outputImage[DispatchRaysIndex().xy] = float4(1.0,0.0,0.0,1.0);
}