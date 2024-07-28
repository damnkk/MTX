; SPIR-V
; Version: 1.6
; Generator: Google spiregg; 0
; Bound: 611
; Schema: 0
               OpCapability RayQueryKHR
               OpCapability RayTracingKHR
               OpCapability RuntimeDescriptorArray
               OpExtension "SPV_KHR_ray_query"
               OpExtension "SPV_KHR_ray_tracing"
               OpExtension "SPV_EXT_descriptor_indexing"
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint RayGenerationNV %raygen "raygen" %3 %4 %matUniformBuffer %vertexBuffer %indexBuffer %instanceInfoBuffer %sceneTextures %primitiveIndexBuffers %Sampler %cameraUniform %RTConstant %outputImage %topLevelAS %envTextures %prd
               OpSource HLSL 650
               OpName %type_StructuredBuffer_MatUniform "type.StructuredBuffer.MatUniform"
               OpName %MatUniform "MatUniform"
               OpMemberName %MatUniform 0 "modelMatrix"
               OpMemberName %MatUniform 1 "baseColorFactor"
               OpMemberName %MatUniform 2 "emissiveFactor"
               OpMemberName %MatUniform 3 "envFactor"
               OpMemberName %MatUniform 4 "mrFactor"
               OpMemberName %MatUniform 5 "intensity"
               OpMemberName %MatUniform 6 "textureUseSetting"
               OpMemberName %MatUniform 7 "textureIndices"
               OpName %matUniformBuffer "matUniformBuffer"
               OpName %type_StructuredBuffer_Vertex "type.StructuredBuffer.Vertex"
               OpName %Vertex "Vertex"
               OpMemberName %Vertex 0 "position"
               OpMemberName %Vertex 1 "normal"
               OpMemberName %Vertex 2 "tangent"
               OpMemberName %Vertex 3 "texcoord"
               OpName %vertexBuffer "vertexBuffer"
               OpName %type_StructuredBuffer_uint "type.StructuredBuffer.uint"
               OpName %indexBuffer "indexBuffer"
               OpName %type_StructuredBuffer_InstanceInfo "type.StructuredBuffer.InstanceInfo"
               OpName %InstanceInfo "InstanceInfo"
               OpMemberName %InstanceInfo 0 "indexOffset"
               OpMemberName %InstanceInfo 1 "vertexOffset"
               OpMemberName %InstanceInfo 2 "vertexCount"
               OpMemberName %InstanceInfo 3 "indexCount"
               OpMemberName %InstanceInfo 4 "primitiveInfoIdx"
               OpName %instanceInfoBuffer "instanceInfoBuffer"
               OpName %type_2d_image "type.2d.image"
               OpName %sceneTextures "sceneTextures"
               OpName %primitiveIndexBuffers "primitiveIndexBuffers"
               OpName %type_sampler "type.sampler"
               OpName %Sampler "Sampler"
               OpName %type_StructuredBuffer_CameraUniform "type.StructuredBuffer.CameraUniform"
               OpName %CameraUniform "CameraUniform"
               OpMemberName %CameraUniform 0 "ViewToClip"
               OpMemberName %CameraUniform 1 "ClipToView"
               OpMemberName %CameraUniform 2 "WorldToView"
               OpMemberName %CameraUniform 3 "ViewToWorld"
               OpMemberName %CameraUniform 4 "WorldToClip"
               OpMemberName %CameraUniform 5 "ClipToWorld"
               OpMemberName %CameraUniform 6 "posFov"
               OpName %cameraUniform "cameraUniform"
               OpName %type_PushConstant_PushConstant "type.PushConstant.PushConstant"
               OpMemberName %type_PushConstant_PushConstant 0 "curFrameCount"
               OpMemberName %type_PushConstant_PushConstant 1 "maxSampleCount"
               OpMemberName %type_PushConstant_PushConstant 2 "maxBounce"
               OpName %RTConstant "RTConstant"
               OpName %type_2d_image_0 "type.2d.image"
               OpName %outputImage "outputImage"
               OpName %accelerationStructureNV "accelerationStructureNV"
               OpName %topLevelAS "topLevelAS"
               OpName %envTextures "envTextures"
               OpName %RayPayloadType "RayPayloadType"
               OpMemberName %RayPayloadType 0 "seed"
               OpMemberName %RayPayloadType 1 "hitT"
               OpMemberName %RayPayloadType 2 "primitiveID"
               OpMemberName %RayPayloadType 3 "instanceID"
               OpMemberName %RayPayloadType 4 "instanceCustomIndex"
               OpMemberName %RayPayloadType 5 "baryCoord"
               OpMemberName %RayPayloadType 6 "objectToWorld"
               OpMemberName %RayPayloadType 7 "worldToObject"
               OpName %prd "prd"
               OpName %raygen "raygen"
               OpName %type_sampled_image "type.sampled.image"
               OpDecorate %3 BuiltIn LaunchIdNV
               OpDecorate %4 BuiltIn LaunchSizeNV
               OpDecorate %prd Location 0
               OpDecorate %matUniformBuffer DescriptorSet 1
               OpDecorate %matUniformBuffer Binding 200
               OpDecorate %vertexBuffer DescriptorSet 1
               OpDecorate %vertexBuffer Binding 201
               OpDecorate %indexBuffer DescriptorSet 1
               OpDecorate %indexBuffer Binding 202
               OpDecorate %instanceInfoBuffer DescriptorSet 1
               OpDecorate %instanceInfoBuffer Binding 203
               OpDecorate %sceneTextures DescriptorSet 2
               OpDecorate %sceneTextures Binding 200
               OpDecorate %primitiveIndexBuffers DescriptorSet 4
               OpDecorate %primitiveIndexBuffers Binding 200
               OpDecorate %Sampler DescriptorSet 1
               OpDecorate %Sampler Binding 104
               OpDecorate %cameraUniform DescriptorSet 0
               OpDecorate %cameraUniform Binding 202
               OpDecorate %outputImage DescriptorSet 0
               OpDecorate %outputImage Binding 400
               OpDecorate %topLevelAS DescriptorSet 0
               OpDecorate %topLevelAS Binding 201
               OpDecorate %envTextures DescriptorSet 3
               OpDecorate %envTextures Binding 200
               OpDecorate %_arr_int_uint_4 ArrayStride 4
               OpDecorate %_arr_int_uint_32 ArrayStride 4
               OpMemberDecorate %MatUniform 0 Offset 0
               OpMemberDecorate %MatUniform 0 MatrixStride 16
               OpMemberDecorate %MatUniform 0 RowMajor
               OpMemberDecorate %MatUniform 1 Offset 64
               OpMemberDecorate %MatUniform 2 Offset 80
               OpMemberDecorate %MatUniform 3 Offset 96
               OpMemberDecorate %MatUniform 4 Offset 112
               OpMemberDecorate %MatUniform 5 Offset 128
               OpMemberDecorate %MatUniform 6 Offset 144
               OpMemberDecorate %MatUniform 7 Offset 160
               OpDecorate %_runtimearr_MatUniform ArrayStride 288
               OpMemberDecorate %type_StructuredBuffer_MatUniform 0 Offset 0
               OpMemberDecorate %type_StructuredBuffer_MatUniform 0 NonWritable
               OpDecorate %type_StructuredBuffer_MatUniform Block
               OpMemberDecorate %Vertex 0 Offset 0
               OpMemberDecorate %Vertex 1 Offset 16
               OpMemberDecorate %Vertex 2 Offset 32
               OpMemberDecorate %Vertex 3 Offset 48
               OpDecorate %_runtimearr_Vertex ArrayStride 64
               OpMemberDecorate %type_StructuredBuffer_Vertex 0 Offset 0
               OpMemberDecorate %type_StructuredBuffer_Vertex 0 NonWritable
               OpDecorate %type_StructuredBuffer_Vertex Block
               OpDecorate %_runtimearr_uint ArrayStride 4
               OpMemberDecorate %type_StructuredBuffer_uint 0 Offset 0
               OpMemberDecorate %type_StructuredBuffer_uint 0 NonWritable
               OpDecorate %type_StructuredBuffer_uint Block
               OpMemberDecorate %InstanceInfo 0 Offset 0
               OpMemberDecorate %InstanceInfo 1 Offset 4
               OpMemberDecorate %InstanceInfo 2 Offset 8
               OpMemberDecorate %InstanceInfo 3 Offset 12
               OpMemberDecorate %InstanceInfo 4 Offset 16
               OpDecorate %_runtimearr_InstanceInfo ArrayStride 20
               OpMemberDecorate %type_StructuredBuffer_InstanceInfo 0 Offset 0
               OpMemberDecorate %type_StructuredBuffer_InstanceInfo 0 NonWritable
               OpDecorate %type_StructuredBuffer_InstanceInfo Block
               OpMemberDecorate %CameraUniform 0 Offset 0
               OpMemberDecorate %CameraUniform 0 MatrixStride 16
               OpMemberDecorate %CameraUniform 0 RowMajor
               OpMemberDecorate %CameraUniform 1 Offset 64
               OpMemberDecorate %CameraUniform 1 MatrixStride 16
               OpMemberDecorate %CameraUniform 1 RowMajor
               OpMemberDecorate %CameraUniform 2 Offset 128
               OpMemberDecorate %CameraUniform 2 MatrixStride 16
               OpMemberDecorate %CameraUniform 2 RowMajor
               OpMemberDecorate %CameraUniform 3 Offset 192
               OpMemberDecorate %CameraUniform 3 MatrixStride 16
               OpMemberDecorate %CameraUniform 3 RowMajor
               OpMemberDecorate %CameraUniform 4 Offset 256
               OpMemberDecorate %CameraUniform 4 MatrixStride 16
               OpMemberDecorate %CameraUniform 4 RowMajor
               OpMemberDecorate %CameraUniform 5 Offset 320
               OpMemberDecorate %CameraUniform 5 MatrixStride 16
               OpMemberDecorate %CameraUniform 5 RowMajor
               OpMemberDecorate %CameraUniform 6 Offset 384
               OpDecorate %_runtimearr_CameraUniform ArrayStride 400
               OpMemberDecorate %type_StructuredBuffer_CameraUniform 0 Offset 0
               OpMemberDecorate %type_StructuredBuffer_CameraUniform 0 NonWritable
               OpDecorate %type_StructuredBuffer_CameraUniform Block
               OpMemberDecorate %type_PushConstant_PushConstant 0 Offset 0
               OpMemberDecorate %type_PushConstant_PushConstant 1 Offset 4
               OpMemberDecorate %type_PushConstant_PushConstant 2 Offset 8
               OpDecorate %type_PushConstant_PushConstant Block
      %float = OpTypeFloat 32
%float_6_28318548 = OpConstant %float 6.28318548
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
    %float_0 = OpConstant %float 0
    %v3float = OpTypeVector %float 3
         %47 = OpConstantComposite %v3float %float_0 %float_0 %float_0
      %int_1 = OpConstant %int 1
       %uint = OpTypeInt 32 0
   %uint_650 = OpConstant %uint 650
    %float_1 = OpConstant %float 1
         %52 = OpConstantComposite %v3float %float_1 %float_0 %float_0
     %uint_0 = OpConstant %uint 0
  %float_0_5 = OpConstant %float 0.5
    %v2float = OpTypeVector %float 2
         %56 = OpConstantComposite %v2float %float_0_5 %float_0_5
     %uint_1 = OpConstant %uint 1
    %float_2 = OpConstant %float 2
         %59 = OpConstantComposite %v2float %float_1 %float_1
    %uint_16 = OpConstant %uint 16
%uint_2654435769 = OpConstant %uint 2654435769
     %uint_4 = OpConstant %uint 4
    %uint_31 = OpConstant %uint 31
%uint_2738958700 = OpConstant %uint 2738958700
     %uint_5 = OpConstant %uint 5
%uint_3355524772 = OpConstant %uint 3355524772
%uint_2911926141 = OpConstant %uint 2911926141
%uint_2123724318 = OpConstant %uint 2123724318
%uint_1065353216 = OpConstant %uint 1065353216
     %uint_9 = OpConstant %uint 9
         %71 = OpConstantComposite %v3float %float_1 %float_1 %float_1
      %int_2 = OpConstant %int 2
%float_1_00000003e_32 = OpConstant %float 1.00000003e+32
     %uint_3 = OpConstant %uint 3
     %uint_2 = OpConstant %uint 2
         %76 = OpConstantComposite %v3float %float_0_5 %float_0_5 %float_0_5
     %int_n1 = OpConstant %int -1
       %bool = OpTypeBool
      %false = OpConstantFalse %bool
%uint_747796405 = OpConstant %uint 747796405
%uint_2891336453 = OpConstant %uint 2891336453
    %uint_28 = OpConstant %uint 28
%uint_277803737 = OpConstant %uint 277803737
    %uint_22 = OpConstant %uint 22
%float_9_99999975en05 = OpConstant %float 9.99999975e-05
   %uint_255 = OpConstant %uint 255
%float_2_20000005 = OpConstant %float 2.20000005
         %88 = OpConstantComposite %v3float %float_2_20000005 %float_2_20000005 %float_2_20000005
%float_0_999989986 = OpConstant %float 0.999989986
  %float_256 = OpConstant %float 256
%float_1_52587891en05 = OpConstant %float 1.52587891e-05
%float_0_03125 = OpConstant %float 0.03125
    %v4float = OpTypeVector %float 4
%mat4v4float = OpTypeMatrix %v4float 4
%_arr_int_uint_4 = OpTypeArray %int %uint_4
    %uint_32 = OpConstant %uint 32
%_arr_int_uint_32 = OpTypeArray %int %uint_32
 %MatUniform = OpTypeStruct %mat4v4float %v4float %v3float %v3float %v3float %v4float %_arr_int_uint_4 %_arr_int_uint_32
%_runtimearr_MatUniform = OpTypeRuntimeArray %MatUniform
%type_StructuredBuffer_MatUniform = OpTypeStruct %_runtimearr_MatUniform
%_ptr_StorageBuffer_type_StructuredBuffer_MatUniform = OpTypePointer StorageBuffer %type_StructuredBuffer_MatUniform
     %Vertex = OpTypeStruct %v3float %v3float %v4float %v2float
%_runtimearr_Vertex = OpTypeRuntimeArray %Vertex
%type_StructuredBuffer_Vertex = OpTypeStruct %_runtimearr_Vertex
%_ptr_StorageBuffer_type_StructuredBuffer_Vertex = OpTypePointer StorageBuffer %type_StructuredBuffer_Vertex
%_runtimearr_uint = OpTypeRuntimeArray %uint
%type_StructuredBuffer_uint = OpTypeStruct %_runtimearr_uint
%_ptr_StorageBuffer_type_StructuredBuffer_uint = OpTypePointer StorageBuffer %type_StructuredBuffer_uint
%InstanceInfo = OpTypeStruct %uint %uint %uint %uint %uint
%_runtimearr_InstanceInfo = OpTypeRuntimeArray %InstanceInfo
%type_StructuredBuffer_InstanceInfo = OpTypeStruct %_runtimearr_InstanceInfo
%_ptr_StorageBuffer_type_StructuredBuffer_InstanceInfo = OpTypePointer StorageBuffer %type_StructuredBuffer_InstanceInfo
%type_2d_image = OpTypeImage %float 2D 2 0 0 1 Unknown
%_runtimearr_type_2d_image = OpTypeRuntimeArray %type_2d_image
%_ptr_UniformConstant__runtimearr_type_2d_image = OpTypePointer UniformConstant %_runtimearr_type_2d_image
%_runtimearr_type_StructuredBuffer_uint = OpTypeRuntimeArray %type_StructuredBuffer_uint
%_ptr_StorageBuffer__runtimearr_type_StructuredBuffer_uint = OpTypePointer StorageBuffer %_runtimearr_type_StructuredBuffer_uint
%type_sampler = OpTypeSampler
%_ptr_UniformConstant_type_sampler = OpTypePointer UniformConstant %type_sampler
%CameraUniform = OpTypeStruct %mat4v4float %mat4v4float %mat4v4float %mat4v4float %mat4v4float %mat4v4float %v4float
%_runtimearr_CameraUniform = OpTypeRuntimeArray %CameraUniform
%type_StructuredBuffer_CameraUniform = OpTypeStruct %_runtimearr_CameraUniform
%_ptr_StorageBuffer_type_StructuredBuffer_CameraUniform = OpTypePointer StorageBuffer %type_StructuredBuffer_CameraUniform
%type_PushConstant_PushConstant = OpTypeStruct %uint %uint %uint
%_ptr_PushConstant_type_PushConstant_PushConstant = OpTypePointer PushConstant %type_PushConstant_PushConstant
%type_2d_image_0 = OpTypeImage %float 2D 2 0 0 2 Rgba32f
%_ptr_UniformConstant_type_2d_image_0 = OpTypePointer UniformConstant %type_2d_image_0
%accelerationStructureNV = OpTypeAccelerationStructureKHR
%_ptr_UniformConstant_accelerationStructureNV = OpTypePointer UniformConstant %accelerationStructureNV
     %v3uint = OpTypeVector %uint 3
%_ptr_Input_v3uint = OpTypePointer Input %v3uint
%mat3v3float = OpTypeMatrix %v3float 3
%RayPayloadType = OpTypeStruct %uint %float %int %int %int %v2float %mat3v3float %mat3v3float
%_ptr_RayPayloadNV_RayPayloadType = OpTypePointer RayPayloadNV %RayPayloadType
       %void = OpTypeVoid
        %114 = OpTypeFunction %void
      %v2int = OpTypeVector %int 2
     %v2uint = OpTypeVector %uint 2
%_ptr_PushConstant_uint = OpTypePointer PushConstant %uint
%_ptr_StorageBuffer_CameraUniform = OpTypePointer StorageBuffer %CameraUniform
%_ptr_Function_accelerationStructureNV = OpTypePointer Function %accelerationStructureNV
%_ptr_UniformConstant_type_2d_image = OpTypePointer UniformConstant %type_2d_image
%type_sampled_image = OpTypeSampledImage %type_2d_image
%_ptr_StorageBuffer_InstanceInfo = OpTypePointer StorageBuffer %InstanceInfo
%_ptr_StorageBuffer_uint = OpTypePointer StorageBuffer %uint
%_ptr_StorageBuffer_Vertex = OpTypePointer StorageBuffer %Vertex
%_ptr_StorageBuffer_MatUniform = OpTypePointer StorageBuffer %MatUniform
%matUniformBuffer = OpVariable %_ptr_StorageBuffer_type_StructuredBuffer_MatUniform StorageBuffer
%vertexBuffer = OpVariable %_ptr_StorageBuffer_type_StructuredBuffer_Vertex StorageBuffer
%indexBuffer = OpVariable %_ptr_StorageBuffer_type_StructuredBuffer_uint StorageBuffer
%instanceInfoBuffer = OpVariable %_ptr_StorageBuffer_type_StructuredBuffer_InstanceInfo StorageBuffer
%sceneTextures = OpVariable %_ptr_UniformConstant__runtimearr_type_2d_image UniformConstant
%primitiveIndexBuffers = OpVariable %_ptr_StorageBuffer__runtimearr_type_StructuredBuffer_uint StorageBuffer
    %Sampler = OpVariable %_ptr_UniformConstant_type_sampler UniformConstant
%cameraUniform = OpVariable %_ptr_StorageBuffer_type_StructuredBuffer_CameraUniform StorageBuffer
 %RTConstant = OpVariable %_ptr_PushConstant_type_PushConstant_PushConstant PushConstant
%outputImage = OpVariable %_ptr_UniformConstant_type_2d_image_0 UniformConstant
 %topLevelAS = OpVariable %_ptr_UniformConstant_accelerationStructureNV UniformConstant
%envTextures = OpVariable %_ptr_UniformConstant__runtimearr_type_2d_image UniformConstant
          %3 = OpVariable %_ptr_Input_v3uint Input
          %4 = OpVariable %_ptr_Input_v3uint Input
        %prd = OpVariable %_ptr_RayPayloadNV_RayPayloadType RayPayloadNV
       %true = OpConstantTrue %bool
        %126 = OpUndef %mat3v3float
        %127 = OpUndef %int
        %128 = OpUndef %float
        %129 = OpUndef %v3float
%float_0_318309873 = OpConstant %float 0.318309873
        %131 = OpConstantComposite %v3float %float_0_318309873 %float_0_318309873 %float_0_318309873
%float_0_159154937 = OpConstant %float 0.159154937
        %133 = OpConstantComposite %v2float %float_0_159154937 %float_0_318309873
        %134 = OpUndef %float
        %135 = OpUndef %v2float
     %v3bool = OpTypeVector %bool 3
     %raygen = OpFunction %void None %114
        %137 = OpLabel
        %138 = OpVariable %_ptr_Function_accelerationStructureNV Function
        %139 = OpLoad %v3uint %3
        %140 = OpVectorShuffle %v2uint %139 %139 0 1
        %141 = OpBitcast %v2int %140
        %142 = OpLoad %v3uint %4
        %143 = OpVectorShuffle %v2uint %142 %142 0 1
        %144 = OpBitcast %v2int %143
        %145 = OpAccessChain %_ptr_PushConstant_uint %RTConstant %int_0
        %146 = OpLoad %uint %145
        %147 = OpCompositeExtract %uint %139 1
        %148 = OpCompositeExtract %uint %142 0
        %149 = OpIMul %uint %147 %148
        %150 = OpCompositeExtract %uint %139 0
        %151 = OpIAdd %uint %149 %150
               OpBranch %152
        %152 = OpLabel
        %153 = OpPhi %uint %146 %137 %154 %155
        %156 = OpPhi %uint %uint_0 %137 %157 %155
        %158 = OpPhi %uint %151 %137 %159 %155
        %160 = OpPhi %uint %uint_0 %137 %161 %155
        %162 = OpULessThan %bool %160 %uint_16
               OpLoopMerge %163 %155 None
               OpBranchConditional %162 %155 %163
        %155 = OpLabel
        %157 = OpIAdd %uint %156 %uint_2654435769
        %164 = OpShiftLeftLogical %uint %153 %uint_4
        %165 = OpIAdd %uint %164 %uint_2738958700
        %166 = OpIAdd %uint %153 %157
        %167 = OpBitwiseXor %uint %165 %166
        %168 = OpShiftRightLogical %uint %153 %uint_5
        %169 = OpIAdd %uint %168 %uint_3355524772
        %170 = OpBitwiseXor %uint %167 %169
        %159 = OpIAdd %uint %158 %170
        %171 = OpShiftLeftLogical %uint %159 %uint_4
        %172 = OpIAdd %uint %171 %uint_2911926141
        %173 = OpIAdd %uint %159 %157
        %174 = OpBitwiseXor %uint %172 %173
        %175 = OpShiftRightLogical %uint %159 %uint_5
        %176 = OpIAdd %uint %175 %uint_2123724318
        %177 = OpBitwiseXor %uint %174 %176
        %154 = OpIAdd %uint %153 %177
        %161 = OpIAdd %uint %160 %uint_1
               OpBranch %152
        %163 = OpLabel
               OpBranch %178
        %178 = OpLabel
        %179 = OpPhi %v2float %135 %163 %180 %181
        %182 = OpPhi %v3float %47 %163 %183 %181
        %184 = OpPhi %v3float %129 %163 %185 %181
        %186 = OpPhi %int %int_0 %163 %187 %181
        %188 = OpSLessThan %bool %186 %int_1
               OpLoopMerge %189 %181 None
               OpBranchConditional %188 %190 %189
        %190 = OpLabel
        %191 = OpIEqual %bool %146 %uint_0
               OpSelectionMerge %192 None
               OpBranchConditional %191 %193 %194
        %193 = OpLabel
               OpBranch %192
        %194 = OpLabel
        %195 = OpIMul %uint %158 %uint_747796405
        %196 = OpIAdd %uint %195 %uint_2891336453
        %197 = OpShiftRightLogical %uint %196 %uint_28
        %198 = OpIAdd %uint %197 %uint_4
        %199 = OpBitwiseAnd %uint %198 %uint_31
        %200 = OpShiftRightLogical %uint %196 %199
        %201 = OpBitwiseXor %uint %200 %196
        %202 = OpIMul %uint %201 %uint_277803737
        %203 = OpShiftRightLogical %uint %202 %uint_22
        %204 = OpBitwiseXor %uint %203 %202
        %205 = OpShiftRightLogical %uint %204 %uint_9
        %206 = OpBitwiseOr %uint %uint_1065353216 %205
        %207 = OpBitcast %float %206
        %208 = OpFSub %float %207 %float_1
        %209 = OpIMul %uint %196 %uint_747796405
        %210 = OpIAdd %uint %209 %uint_2891336453
        %211 = OpShiftRightLogical %uint %210 %uint_28
        %212 = OpIAdd %uint %211 %uint_4
        %213 = OpBitwiseAnd %uint %212 %uint_31
        %214 = OpShiftRightLogical %uint %210 %213
        %215 = OpBitwiseXor %uint %214 %210
        %216 = OpIMul %uint %215 %uint_277803737
        %217 = OpShiftRightLogical %uint %216 %uint_22
        %218 = OpBitwiseXor %uint %217 %216
        %219 = OpShiftRightLogical %uint %218 %uint_9
        %220 = OpBitwiseOr %uint %uint_1065353216 %219
        %221 = OpBitcast %float %220
        %222 = OpFSub %float %221 %float_1
        %223 = OpCompositeConstruct %v2float %208 %222
               OpBranch %192
        %192 = OpLabel
        %224 = OpPhi %uint %158 %193 %210 %194
        %225 = OpPhi %v2float %56 %193 %223 %194
        %226 = OpConvertSToF %v2float %141
        %227 = OpFAdd %v2float %226 %225
        %228 = OpConvertSToF %v2float %144
        %229 = OpFDiv %v2float %227 %228
        %230 = OpCompositeExtract %float %229 1
        %231 = OpFSub %float %float_1 %230
        %232 = OpCompositeInsert %v2float %231 %229 1
        %233 = OpVectorTimesScalar %v2float %232 %float_2
        %234 = OpFSub %v2float %233 %59
        %235 = OpAccessChain %_ptr_StorageBuffer_CameraUniform %cameraUniform %int_0 %uint_0
        %236 = OpLoad %CameraUniform %235
        %237 = OpCompositeExtract %mat4v4float %236 1
        %238 = OpCompositeExtract %mat4v4float %236 3
        %239 = OpCompositeExtract %v4float %236 6
        %240 = OpCompositeExtract %float %234 0
        %241 = OpCompositeExtract %float %234 1
        %242 = OpCompositeConstruct %v4float %240 %241 %float_1 %float_1
        %243 = OpVectorTimesMatrix %v4float %242 %237
        %244 = OpVectorShuffle %v3float %243 %243 0 1 2
        %245 = OpExtInst %v3float %1 Normalize %244
        %246 = OpCompositeExtract %float %245 0
        %247 = OpCompositeExtract %float %245 1
        %248 = OpCompositeExtract %float %245 2
        %249 = OpCompositeConstruct %v4float %246 %247 %248 %float_0
        %250 = OpVectorTimesMatrix %v4float %249 %238
        %251 = OpVectorShuffle %v3float %239 %239 0 1 2
        %252 = OpVectorShuffle %v3float %250 %250 0 1 2
               OpSelectionMerge %253 None
               OpSwitch %uint_0 %254
        %254 = OpLabel
               OpBranch %255
        %255 = OpLabel
        %256 = OpPhi %v2float %179 %254 %257 %258
        %259 = OpPhi %v3float %71 %254 %260 %258
        %261 = OpPhi %uint %224 %254 %262 %258
        %263 = OpPhi %float %128 %254 %264 %258
        %265 = OpPhi %int %127 %254 %266 %258
        %267 = OpPhi %int %127 %254 %268 %258
        %269 = OpPhi %int %127 %254 %270 %258
        %271 = OpPhi %mat3v3float %126 %254 %272 %258
        %273 = OpPhi %mat3v3float %126 %254 %274 %258
        %275 = OpPhi %v3float %252 %254 %276 %258
        %277 = OpPhi %v3float %251 %254 %278 %258
        %279 = OpPhi %int %int_0 %254 %280 %258
        %281 = OpBitcast %uint %279
        %282 = OpAccessChain %_ptr_PushConstant_uint %RTConstant %int_2
        %283 = OpLoad %uint %282
        %284 = OpULessThan %bool %281 %283
               OpLoopMerge %285 %258 None
               OpBranchConditional %284 %286 %285
        %286 = OpLabel
        %287 = OpLoad %accelerationStructureNV %topLevelAS
               OpStore %138 %287
        %288 = OpCompositeConstruct %RayPayloadType %261 %263 %265 %267 %269 %256 %271 %273
               OpStore %prd %288
        %289 = OpLoad %accelerationStructureNV %138
               OpTraceRayKHR %289 %uint_1 %uint_255 %uint_0 %uint_1 %uint_0 %277 %float_9_99999975en05 %275 %float_1_00000003e_32 %prd
        %290 = OpLoad %RayPayloadType %prd
        %291 = OpCompositeExtract %uint %290 0
        %264 = OpCompositeExtract %float %290 1
        %266 = OpCompositeExtract %int %290 2
        %268 = OpCompositeExtract %int %290 3
        %270 = OpCompositeExtract %int %290 4
        %257 = OpCompositeExtract %v2float %290 5
        %272 = OpCompositeExtract %mat3v3float %290 6
        %274 = OpCompositeExtract %mat3v3float %290 7
        %292 = OpFOrdEqual %bool %264 %float_1_00000003e_32
               OpSelectionMerge %293 None
               OpBranchConditional %292 %294 %293
        %294 = OpLabel
        %295 = OpExtInst %v3float %1 Normalize %275
        %296 = OpCompositeExtract %float %295 1
        %297 = OpCompositeExtract %float %295 0
        %298 = OpExtInst %float %1 Atan2 %296 %297
        %299 = OpCompositeExtract %float %295 2
        %300 = OpExtInst %float %1 Asin %299
        %301 = OpCompositeConstruct %v2float %298 %300
        %302 = OpExtInst %v2float %1 Fma %301 %133 %56
        %303 = OpCompositeExtract %float %302 1
        %304 = OpFSub %float %float_1 %303
        %305 = OpCompositeInsert %v2float %304 %302 1
        %306 = OpAccessChain %_ptr_UniformConstant_type_2d_image %envTextures %int_0
        %307 = OpLoad %type_2d_image %306
        %308 = OpLoad %type_sampler %Sampler
        %309 = OpSampledImage %type_sampled_image %307 %308
        %310 = OpImageSampleExplicitLod %v4float %309 %305 Lod %float_0
        %311 = OpVectorShuffle %v3float %310 %310 0 1 2
        %312 = OpFMul %v3float %311 %259
               OpBranch %285
        %293 = OpLabel
        %313 = OpBitcast %uint %268
        %314 = OpBitcast %uint %266
        %315 = OpAccessChain %_ptr_StorageBuffer_InstanceInfo %instanceInfoBuffer %int_0 %313
        %316 = OpLoad %InstanceInfo %315
        %317 = OpCompositeExtract %uint %316 0
        %318 = OpCompositeExtract %uint %316 1
        %319 = OpCompositeExtract %uint %316 4
        %320 = OpIMul %uint %uint_3 %314
        %321 = OpIAdd %uint %317 %320
        %322 = OpAccessChain %_ptr_StorageBuffer_uint %indexBuffer %int_0 %321
        %323 = OpLoad %uint %322
        %324 = OpIAdd %uint %321 %uint_1
        %325 = OpAccessChain %_ptr_StorageBuffer_uint %indexBuffer %int_0 %324
        %326 = OpLoad %uint %325
        %327 = OpIAdd %uint %321 %uint_2
        %328 = OpAccessChain %_ptr_StorageBuffer_uint %indexBuffer %int_0 %327
        %329 = OpLoad %uint %328
        %330 = OpIAdd %uint %318 %323
        %331 = OpAccessChain %_ptr_StorageBuffer_Vertex %vertexBuffer %int_0 %330
        %332 = OpLoad %Vertex %331
        %333 = OpCompositeExtract %v3float %332 0
        %334 = OpCompositeExtract %v3float %332 1
        %335 = OpCompositeExtract %v4float %332 2
        %336 = OpCompositeExtract %v2float %332 3
        %337 = OpIAdd %uint %318 %326
        %338 = OpAccessChain %_ptr_StorageBuffer_Vertex %vertexBuffer %int_0 %337
        %339 = OpLoad %Vertex %338
        %340 = OpCompositeExtract %v3float %339 0
        %341 = OpCompositeExtract %v3float %339 1
        %342 = OpCompositeExtract %v4float %339 2
        %343 = OpCompositeExtract %v2float %339 3
        %344 = OpIAdd %uint %318 %329
        %345 = OpAccessChain %_ptr_StorageBuffer_Vertex %vertexBuffer %int_0 %344
        %346 = OpLoad %Vertex %345
        %347 = OpCompositeExtract %v3float %346 0
        %348 = OpCompositeExtract %v3float %346 1
        %349 = OpCompositeExtract %v4float %346 2
        %350 = OpCompositeExtract %v2float %346 3
        %351 = OpCompositeExtract %float %257 0
        %352 = OpFSub %float %float_1 %351
        %353 = OpCompositeExtract %float %257 1
        %354 = OpFSub %float %352 %353
        %355 = OpVectorTimesScalar %v2float %336 %354
        %356 = OpVectorTimesScalar %v2float %343 %351
        %357 = OpFAdd %v2float %355 %356
        %358 = OpVectorTimesScalar %v2float %350 %353
        %359 = OpFAdd %v2float %357 %358
        %360 = OpVectorTimesScalar %v3float %333 %354
        %361 = OpVectorTimesScalar %v3float %340 %351
        %362 = OpFAdd %v3float %360 %361
        %363 = OpVectorTimesScalar %v3float %347 %353
        %364 = OpFAdd %v3float %362 %363
        %365 = OpVectorTimesMatrix %v3float %364 %272
        %366 = OpVectorTimesScalar %v3float %334 %354
        %367 = OpVectorTimesScalar %v3float %341 %351
        %368 = OpFAdd %v3float %366 %367
        %369 = OpVectorTimesScalar %v3float %348 %353
        %370 = OpFAdd %v3float %368 %369
        %371 = OpVectorTimesMatrix %v3float %370 %272
        %372 = OpVectorShuffle %v3float %335 %335 0 1 2
        %373 = OpVectorTimesScalar %v3float %372 %354
        %374 = OpVectorShuffle %v3float %342 %342 0 1 2
        %375 = OpVectorTimesScalar %v3float %374 %351
        %376 = OpFAdd %v3float %373 %375
        %377 = OpVectorShuffle %v3float %349 %349 0 1 2
        %378 = OpVectorTimesScalar %v3float %377 %353
        %379 = OpFAdd %v3float %376 %378
        %380 = OpExtInst %v3float %1 Normalize %379
        %381 = OpVectorTimesMatrix %v3float %380 %272
        %382 = OpExtInst %v3float %1 Normalize %381
        %383 = OpAccessChain %_ptr_StorageBuffer_uint %primitiveIndexBuffers %319 %int_0 %314
        %384 = OpLoad %uint %383
        %385 = OpAccessChain %_ptr_StorageBuffer_MatUniform %matUniformBuffer %int_0 %384
        %386 = OpLoad %MatUniform %385
        %387 = OpCompositeExtract %_arr_int_uint_32 %386 7
        %388 = OpCompositeExtract %int %387 0
        %389 = OpCompositeExtract %int %387 3
        %390 = OpSGreaterThan %bool %388 %int_n1
               OpSelectionMerge %391 None
               OpBranchConditional %390 %392 %391
        %392 = OpLabel
        %393 = OpAccessChain %_ptr_UniformConstant_type_2d_image %sceneTextures %388
        %394 = OpLoad %type_2d_image %393
        %395 = OpLoad %type_sampler %Sampler
        %396 = OpSampledImage %type_sampled_image %394 %395
        %397 = OpImageSampleExplicitLod %v4float %396 %359 Lod %float_0
        %398 = OpVectorShuffle %v3float %397 %397 0 1 2
        %399 = OpExtInst %v3float %1 Pow %398 %88
        %400 = OpCompositeExtract %float %399 0
        %401 = OpCompositeExtract %float %399 1
        %402 = OpCompositeExtract %float %399 2
        %403 = OpCompositeConstruct %v4float %400 %401 %402 %134
        %404 = OpVectorShuffle %v3float %403 %403 0 1 2
               OpBranch %391
        %391 = OpLabel
        %405 = OpPhi %v3float %76 %293 %404 %392
        %406 = OpExtInst %v3float %1 Normalize %371
        %407 = OpSGreaterThan %bool %389 %int_n1
               OpSelectionMerge %408 None
               OpBranchConditional %407 %409 %408
        %409 = OpLabel
        %410 = OpAccessChain %_ptr_UniformConstant_type_2d_image %sceneTextures %389
        %411 = OpLoad %type_2d_image %410
        %412 = OpLoad %type_sampler %Sampler
        %413 = OpSampledImage %type_sampled_image %411 %412
        %414 = OpImageSampleExplicitLod %v4float %413 %359 Lod %float_0
        %415 = OpExtInst %v3float %1 Cross %371 %382
        %416 = OpExtInst %v3float %1 Normalize %415
        %417 = OpCompositeExtract %float %414 0
        %418 = OpVectorTimesScalar %v3float %382 %417
        %419 = OpCompositeExtract %float %414 1
        %420 = OpVectorTimesScalar %v3float %416 %419
        %421 = OpFAdd %v3float %418 %420
        %422 = OpCompositeExtract %float %414 2
        %423 = OpVectorTimesScalar %v3float %371 %422
        %424 = OpFAdd %v3float %421 %423
        %425 = OpExtInst %v3float %1 Normalize %424
               OpBranch %408
        %408 = OpLabel
        %426 = OpPhi %v3float %406 %391 %425 %409
        %427 = OpDot %float %426 %275
        %428 = OpFOrdLessThanEqual %bool %427 %float_0
               OpSelectionMerge %429 None
               OpBranchConditional %428 %430 %431
        %430 = OpLabel
               OpBranch %429
        %431 = OpLabel
        %432 = OpFNegate %v3float %426
               OpBranch %429
        %429 = OpLabel
        %433 = OpPhi %v3float %426 %430 %432 %431
        %434 = OpIMul %uint %291 %uint_747796405
        %435 = OpIAdd %uint %434 %uint_2891336453
        %436 = OpShiftRightLogical %uint %435 %uint_28
        %437 = OpIAdd %uint %436 %uint_4
        %438 = OpBitwiseAnd %uint %437 %uint_31
        %439 = OpShiftRightLogical %uint %435 %438
        %440 = OpBitwiseXor %uint %439 %435
        %441 = OpIMul %uint %440 %uint_277803737
        %442 = OpShiftRightLogical %uint %441 %uint_22
        %443 = OpBitwiseXor %uint %442 %441
        %444 = OpShiftRightLogical %uint %443 %uint_9
        %445 = OpBitwiseOr %uint %uint_1065353216 %444
        %446 = OpBitcast %float %445
        %447 = OpFSub %float %446 %float_1
        %448 = OpIMul %uint %435 %uint_747796405
        %262 = OpIAdd %uint %448 %uint_2891336453
        %449 = OpShiftRightLogical %uint %262 %uint_28
        %450 = OpIAdd %uint %449 %uint_4
        %451 = OpBitwiseAnd %uint %450 %uint_31
        %452 = OpShiftRightLogical %uint %262 %451
        %453 = OpBitwiseXor %uint %452 %262
        %454 = OpIMul %uint %453 %uint_277803737
        %455 = OpShiftRightLogical %uint %454 %uint_22
        %456 = OpBitwiseXor %uint %455 %454
        %457 = OpShiftRightLogical %uint %456 %uint_9
        %458 = OpBitwiseOr %uint %uint_1065353216 %457
        %459 = OpBitcast %float %458
        %460 = OpFSub %float %459 %float_1
        %461 = OpCompositeExtract %float %433 2
        %462 = OpExtInst %float %1 FAbs %461
        %463 = OpFOrdGreaterThan %bool %462 %float_0_999989986
               OpSelectionMerge %464 None
               OpBranchConditional %463 %465 %466
        %465 = OpLabel
        %467 = OpCompositeExtract %float %433 0
        %468 = OpFNegate %float %467
        %469 = OpCompositeExtract %float %433 1
        %470 = OpFMul %float %468 %469
        %471 = OpFNegate %float %469
        %472 = OpExtInst %float %1 Fma %471 %469 %float_1
        %473 = OpFMul %float %471 %461
        %474 = OpCompositeConstruct %v3float %470 %472 %473
               OpBranch %464
        %466 = OpLabel
        %475 = OpCompositeExtract %float %433 0
        %476 = OpFNegate %float %475
        %477 = OpFMul %float %476 %461
        %478 = OpCompositeExtract %float %433 1
        %479 = OpFNegate %float %478
        %480 = OpFMul %float %479 %461
        %481 = OpFNegate %float %461
        %482 = OpExtInst %float %1 Fma %481 %461 %float_1
        %483 = OpCompositeConstruct %v3float %477 %480 %482
               OpBranch %464
        %464 = OpLabel
        %484 = OpPhi %v3float %474 %465 %483 %466
        %485 = OpExtInst %v3float %1 Normalize %484
        %486 = OpExtInst %v3float %1 Cross %485 %433
        %487 = OpExtInst %v3float %1 Normalize %486
        %488 = OpExtInst %float %1 Sqrt %447
        %489 = OpFMul %float %float_6_28318548 %460
        %490 = OpExtInst %float %1 Cos %489
        %491 = OpFMul %float %490 %488
        %492 = OpExtInst %float %1 Sin %489
        %493 = OpFMul %float %492 %488
        %494 = OpFSub %float %float_2 %446
        %495 = OpExtInst %float %1 Sqrt %494
        %496 = OpVectorTimesScalar %v3float %485 %491
        %497 = OpVectorTimesScalar %v3float %487 %493
        %498 = OpFAdd %v3float %496 %497
        %499 = OpVectorTimesScalar %v3float %433 %495
        %276 = OpFAdd %v3float %498 %499
        %500 = OpDot %float %276 %433
        %501 = OpFMul %float %500 %float_0_318309873
        %502 = OpFMul %v3float %405 %131
        %503 = OpFOrdGreaterThan %bool %501 %float_0
               OpSelectionMerge %504 None
               OpBranchConditional %503 %504 %505
        %504 = OpLabel
        %506 = OpDot %float %433 %276
        %507 = OpExtInst %float %1 FAbs %506
        %508 = OpVectorTimesScalar %v3float %502 %507
        %509 = OpCompositeConstruct %v3float %501 %501 %501
        %510 = OpFDiv %v3float %508 %509
        %260 = OpFMul %v3float %259 %510
        %511 = OpFOrdGreaterThan %bool %500 %float_0
               OpSelectionMerge %512 None
               OpBranchConditional %511 %513 %514
        %505 = OpLabel
               OpBranch %285
        %513 = OpLabel
               OpBranch %512
        %514 = OpLabel
        %515 = OpFNegate %v3float %433
               OpBranch %512
        %512 = OpLabel
        %516 = OpPhi %v3float %433 %513 %515 %514
        %517 = OpCompositeExtract %float %516 0
        %518 = OpFMul %float %float_256 %517
        %519 = OpConvertFToS %int %518
        %520 = OpCompositeExtract %float %516 1
        %521 = OpFMul %float %float_256 %520
        %522 = OpConvertFToS %int %521
        %523 = OpCompositeExtract %float %516 2
        %524 = OpFMul %float %float_256 %523
        %525 = OpConvertFToS %int %524
        %526 = OpCompositeExtract %float %365 0
        %527 = OpBitcast %int %526
        %528 = OpFOrdLessThan %bool %526 %float_0
               OpSelectionMerge %529 None
               OpBranchConditional %528 %530 %531
        %530 = OpLabel
        %532 = OpSNegate %int %519
               OpBranch %529
        %531 = OpLabel
               OpBranch %529
        %529 = OpLabel
        %533 = OpPhi %int %532 %530 %519 %531
        %534 = OpIAdd %int %527 %533
        %535 = OpBitcast %float %534
        %536 = OpCompositeExtract %float %365 1
        %537 = OpBitcast %int %536
        %538 = OpFOrdLessThan %bool %536 %float_0
               OpSelectionMerge %539 None
               OpBranchConditional %538 %540 %541
        %540 = OpLabel
        %542 = OpSNegate %int %522
               OpBranch %539
        %541 = OpLabel
               OpBranch %539
        %539 = OpLabel
        %543 = OpPhi %int %542 %540 %522 %541
        %544 = OpIAdd %int %537 %543
        %545 = OpBitcast %float %544
        %546 = OpCompositeExtract %float %365 2
        %547 = OpBitcast %int %546
        %548 = OpFOrdLessThan %bool %546 %float_0
               OpSelectionMerge %549 None
               OpBranchConditional %548 %550 %551
        %550 = OpLabel
        %552 = OpSNegate %int %525
               OpBranch %549
        %551 = OpLabel
               OpBranch %549
        %549 = OpLabel
        %553 = OpPhi %int %552 %550 %525 %551
        %554 = OpIAdd %int %547 %553
        %555 = OpBitcast %float %554
        %556 = OpExtInst %float %1 FAbs %526
        %557 = OpFOrdLessThan %bool %556 %float_0_03125
               OpSelectionMerge %558 None
               OpBranchConditional %557 %559 %560
        %559 = OpLabel
        %561 = OpExtInst %float %1 Fma %float_1_52587891en05 %517 %526
               OpBranch %558
        %560 = OpLabel
               OpBranch %558
        %558 = OpLabel
        %562 = OpPhi %float %561 %559 %535 %560
        %563 = OpExtInst %float %1 FAbs %536
        %564 = OpFOrdLessThan %bool %563 %float_0_03125
               OpSelectionMerge %565 None
               OpBranchConditional %564 %566 %567
        %566 = OpLabel
        %568 = OpExtInst %float %1 Fma %float_1_52587891en05 %520 %536
               OpBranch %565
        %567 = OpLabel
               OpBranch %565
        %565 = OpLabel
        %569 = OpPhi %float %568 %566 %545 %567
        %570 = OpExtInst %float %1 FAbs %546
        %571 = OpFOrdLessThan %bool %570 %float_0_03125
               OpSelectionMerge %572 None
               OpBranchConditional %571 %573 %574
        %573 = OpLabel
        %575 = OpExtInst %float %1 Fma %float_1_52587891en05 %523 %546
               OpBranch %572
        %574 = OpLabel
               OpBranch %572
        %572 = OpLabel
        %576 = OpPhi %float %575 %573 %555 %574
        %278 = OpCompositeConstruct %v3float %562 %569 %576
               OpBranch %258
        %258 = OpLabel
        %280 = OpIAdd %int %279 %int_1
               OpBranch %255
        %285 = OpLabel
        %180 = OpPhi %v2float %256 %255 %257 %294 %257 %505
        %577 = OpPhi %v3float %184 %255 %312 %294 %184 %505
        %578 = OpPhi %bool %false %255 %true %294 %false %505
               OpSelectionMerge %579 None
               OpBranchConditional %578 %253 %579
        %579 = OpLabel
               OpBranch %253
        %253 = OpLabel
        %185 = OpPhi %v3float %577 %285 %47 %579
        %183 = OpFAdd %v3float %182 %185
               OpBranch %181
        %181 = OpLabel
        %187 = OpIAdd %int %186 %int_1
               OpBranch %178
        %189 = OpLabel
        %580 = OpUGreaterThan %bool %146 %uint_650
        %581 = OpCompositeConstruct %v3bool %580 %580 %580
        %582 = OpSelect %v3float %581 %52 %182
        %583 = OpUGreaterThan %bool %146 %uint_0
               OpSelectionMerge %584 None
               OpBranchConditional %583 %585 %586
        %585 = OpLabel
        %587 = OpBitcast %v2uint %141
        %588 = OpLoad %type_2d_image_0 %outputImage
        %589 = OpImageRead %v4float %588 %587 None
        %590 = OpVectorShuffle %v3float %589 %589 0 1 2
        %591 = OpConvertUToF %float %146
        %592 = OpFDiv %float %float_1 %591
        %593 = OpCompositeConstruct %v3float %592 %592 %592
        %594 = OpExtInst %v3float %1 FMix %590 %582 %593
        %595 = OpCompositeExtract %float %594 0
        %596 = OpCompositeExtract %float %582 0
        %597 = OpExtInst %float %1 FMix %595 %596 %592
        %598 = OpCompositeExtract %float %594 1
        %599 = OpCompositeExtract %float %594 2
        %600 = OpCompositeConstruct %v4float %597 %598 %599 %float_1
        %601 = OpLoad %type_2d_image_0 %outputImage
               OpImageWrite %601 %587 %600 None
               OpBranch %584
        %586 = OpLabel
        %602 = OpIEqual %bool %146 %uint_0
               OpSelectionMerge %603 None
               OpBranchConditional %602 %604 %603
        %604 = OpLabel
        %605 = OpCompositeExtract %float %582 0
        %606 = OpCompositeExtract %float %582 1
        %607 = OpCompositeExtract %float %582 2
        %608 = OpCompositeConstruct %v4float %605 %606 %607 %float_1
        %609 = OpBitcast %v2uint %141
        %610 = OpLoad %type_2d_image_0 %outputImage
               OpImageWrite %610 %609 %608 None
               OpBranch %603
        %603 = OpLabel
               OpBranch %584
        %584 = OpLabel
               OpReturn
               OpFunctionEnd
