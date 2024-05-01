#version 460

#extension GL_GOOGLE_include_directive : require

#include "uniforms.glsl"

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = vec4(light.color.xyz, 1.0);
}
