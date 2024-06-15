#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout : require

#include "uniforms.glsl"

uint MaterialFeatures_ColorTexture     = 1 << 0;
uint MaterialFeatures_NormalTexture    = 1 << 1;
uint MaterialFeatures_RoughnessTexture = 1 << 2;
uint MaterialFeatures_OcclusionTexture = 1 << 3;
uint MaterialFeatures_EmissiveTexture =  1 << 4;
uint MaterialFeatures_TangentVertexAttribute = 1 << 5;
uint MaterialFeatures_TexcoordVertexAttribute = 1 << 6;

// NOTE(aether) potential performance hit
layout(buffer_reference, std430) readonly buffer PositionBuffer {
	vec4 positions[];
};

layout(buffer_reference, std430) readonly buffer TangentBuffer {
	vec4 tangents[];
};

layout(buffer_reference, scalar) readonly buffer NormalBuffer {
	vec3 normals[];
};

layout(buffer_reference, std430) readonly buffer TexcoordBuffer {
	vec2 texcoords0[];
};

layout(push_constant) uniform PushConstants
{
    mat4 constmodel;

    PositionBuffer positionBuffer;
    TangentBuffer tangentBuffer;
    NormalBuffer normalbuffer;
    TexcoordBuffer texcoordBuffer;

    uint positionOffset;
    uint tangentOffset;
    uint normalOffset;
    uint texcoordOffset;
};

layout(std140, set = 1, binding = 5) uniform MaterialConstant {
    vec4 base_color_factor;
    mat4 model;
    mat4 model_inv;

    vec3  emissive_factor;
    float metallic_factor;

    float roughness_factor;
    float occlusion_factor;
    uint  flags;
    uint  pad;
};

layout (location = 0) out vec2 vTexcoord0;
layout (location = 1) out vec3 vNormal;
layout (location = 2) out vec4 vTangent;
layout (location = 3) out vec4 vPosition;

void main() {
    vec3 position = positionBuffer.positions[positionOffset + gl_VertexIndex].xyz;
    vec3 normal = normalbuffer.normals[normalOffset + gl_VertexIndex];
    vec4 tangent = tangentBuffer.tangents[tangentOffset + gl_VertexIndex];
    vec2 texcoord = texcoordBuffer.texcoords0[texcoordOffset + gl_VertexIndex];

    gl_Position = sceneData.viewProj * constmodel * vec4(position, 1.0);
    vPosition = constmodel * vec4(position, 1.0);

    if ( ( flags & MaterialFeatures_TexcoordVertexAttribute ) != 0 ) {
        vTexcoord0 = texcoord;
    }
    vNormal = mat3( model_inv ) * normal;

    if ( ( flags & MaterialFeatures_TangentVertexAttribute ) != 0 ) {
        vTangent = tangent;
    }
}
