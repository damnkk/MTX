struct OutputVs{
    float4 position:SV_Position;
    float2 texcoord:TEXCOORD0;
};

OutputVs main(in uint vid:SV_VERTEXID){
    float2 outUV = float2((vid<<1)&2,vid&2);
    OutputVs output;
    output.texcoord = outUV;
    output.position = float4(outUV*2.0-1.0,0.0,1.0);
    return output;
} 
