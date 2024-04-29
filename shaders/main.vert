#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "uniforms.glsl"

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outFragPos;
layout (location = 2) out vec3 outNormal;

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

layout(push_constant) uniform PushConstants
{
	mat4 model;
	VertexBuffer vertexBuffer;
} constants;

void main()
{
	Vertex vertex = constants.vertexBuffer.vertices[gl_VertexIndex];

	gl_Position = sceneData.viewProj * constants.model * vec4(vertex.position, 1.0f);

	outUV.x = vertex.uv_x;
	outUV.y = vertex.uv_y;

	outNormal = mat3(transpose(inverse(constants.model))) * vertex.normal;
	outFragPos = vec3(constants.model * vec4(vertex.position, 1.0f));
}
