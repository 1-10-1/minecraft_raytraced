#version 460

#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec2 outUV;

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

layout(binding = 0) uniform SceneData {
	mat4 view;
	mat4 proj;
	mat4 viewProj;
	vec4 ambientColor;
	vec3 sunlightDirection;
	float sunPower;
	vec4 sunlightColor;
} sceneData;

void main()
{
	Vertex vertex = constants.vertexBuffer.vertices[gl_VertexIndex];

	gl_Position = sceneData.viewProj * constants.model * vec4(vertex.position, 1.0f);

	outUV.x = vertex.uv_x;
	outUV.y = vertex.uv_y;
}
