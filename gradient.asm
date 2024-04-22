; SPIR-V
; Version: 1.6
; Generator: Google Shaderc over Glslang; 11
; Bound: 99
; Schema: 0
               OpCapability Shader
               OpCapability ImageQuery
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %4 "main" %gl_GlobalInvocationID %22 %gl_LocalInvocationID
               OpExecutionMode %4 LocalSize 32 32 1
               OpDecorate %gl_GlobalInvocationID BuiltIn GlobalInvocationId
               OpDecorate %22 DescriptorSet 0
               OpDecorate %22 Binding 0
               OpDecorate %gl_LocalInvocationID BuiltIn LocalInvocationId
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %int = OpTypeInt 32 1
      %v2int = OpTypeVector %int 2
       %uint = OpTypeInt 32 0
     %v3uint = OpTypeVector %uint 3
%_ptr_Input_v3uint = OpTypePointer Input %v3uint
%gl_GlobalInvocationID = OpVariable %_ptr_Input_v3uint Input
     %v2uint = OpTypeVector %uint 2
      %float = OpTypeFloat 32
         %20 = OpTypeImage %float 2D 0 0 0 2 Rgba16f
%_ptr_UniformConstant_20 = OpTypePointer UniformConstant %20
         %22 = OpVariable %_ptr_UniformConstant_20 UniformConstant
       %bool = OpTypeBool
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
    %v4float = OpTypeVector %float 4
    %float_0 = OpConstant %float 0
    %float_1 = OpConstant %float 1
         %49 = OpConstantComposite %v4float %float_0 %float_0 %float_0 %float_1
%gl_LocalInvocationID = OpVariable %_ptr_Input_v3uint Input
%_ptr_Input_uint = OpTypePointer Input %uint
          %4 = OpFunction %void None %3
          %5 = OpLabel
         %15 = OpLoad %v3uint %gl_GlobalInvocationID
         %16 = OpVectorShuffle %v2uint %15 %15 0 1
         %17 = OpBitcast %v2int %16
         %23 = OpLoad %20 %22
         %24 = OpImageQuerySize %v2int %23
         %29 = OpCompositeExtract %int %17 0
         %31 = OpCompositeExtract %int %24 0
         %32 = OpSLessThan %bool %29 %31
               OpSelectionMerge %34 None
               OpBranchConditional %32 %33 %34
         %33 = OpLabel
         %37 = OpCompositeExtract %int %17 1
         %39 = OpCompositeExtract %int %24 1
         %40 = OpSLessThan %bool %37 %39
               OpBranch %34
         %34 = OpLabel
         %41 = OpPhi %bool %32 %5 %40 %33
               OpSelectionMerge %43 None
               OpBranchConditional %41 %42 %43
         %42 = OpLabel
         %52 = OpAccessChain %_ptr_Input_uint %gl_LocalInvocationID %uint_0
         %53 = OpLoad %uint %52
         %54 = OpINotEqual %bool %53 %uint_0
               OpSelectionMerge %56 None
               OpBranchConditional %54 %55 %56
         %55 = OpLabel
         %57 = OpAccessChain %_ptr_Input_uint %gl_LocalInvocationID %uint_1
         %58 = OpLoad %uint %57
         %59 = OpINotEqual %bool %58 %uint_0
               OpBranch %56
         %56 = OpLabel
         %60 = OpPhi %bool %54 %42 %59 %55
               OpSelectionMerge %62 None
               OpBranchConditional %60 %61 %62
         %61 = OpLabel
         %65 = OpConvertSToF %float %29
         %68 = OpConvertSToF %float %31
         %69 = OpFDiv %float %65 %68
         %92 = OpCompositeInsert %v4float %69 %49 0
         %73 = OpCompositeExtract %int %17 1
         %74 = OpConvertSToF %float %73
         %76 = OpCompositeExtract %int %24 1
         %77 = OpConvertSToF %float %76
         %78 = OpFDiv %float %74 %77
         %96 = OpCompositeInsert %v4float %78 %92 1
               OpBranch %62
         %62 = OpLabel
         %98 = OpPhi %v4float %49 %56 %96 %61
         %80 = OpLoad %20 %22
               OpImageWrite %80 %17 %98
               OpBranch %43
         %43 = OpLabel
               OpReturn
               OpFunctionEnd
