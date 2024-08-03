#include "NRICompatibility.hlsli"

NRI_RESOURCE(Texture2D, RTRes,t,0,0);
NRI_RESOURCE(SamplerState,SSampler,s,1,0);

struct PushConstants{
    float exposure ;
    float brightness ;
    float contrast ;
    float saturation;
    float vignette ;
};
struct OutputVs{
    float4 position:SV_Position;
    float2 texcoord:TEXCOORD0;
};

float4 main(in OutputVs input):SV_Target{
    // float4 RTResColor = RTRes.Sample(SSampler,input.texcoord);
    float4 RTResColor = float4(1.0,0.0,0.0,1.0);
    return RTResColor;
}