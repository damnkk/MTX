NRI_RESOURCE(StructuredBuffer<MatUniform>, matUniformBuffer, t, 0, 1);
NRI_RESOURCE(StructuredBuffer<Vertex>, vertexBuffer, t, 1, 1);
NRI_RESOURCE(StructuredBuffer<uint>, indexBuffer, t, 2, 1);
NRI_RESOURCE(StructuredBuffer<InstanceInfo>, instanceInfoBuffer, t, 3, 1);
NRI_RESOURCE(Texture2D<float4>, sceneTextures[], t, 0, 2);
NRI_RESOURCE(StructuredBuffer<uint>, primitiveIndexBuffers[], t, 0, 4);
NRI_RESOURCE(SamplerState, Sampler, s, 4, 1);
NRI_RESOURCE(StructuredBuffer<CameraUniform>, cameraUniform, t, 2, 0);
NRI_PUSH_CONSTANTS(PushConstant, RTConstant, 0);
