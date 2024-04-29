#version 460

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler2D colorTex;

void main() 
{
	outFragColor = texture(colorTex, inUV);
}
