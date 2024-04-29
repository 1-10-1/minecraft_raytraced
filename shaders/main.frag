#version 460

#extension GL_GOOGLE_include_directive : require

#include "uniforms.glsl"

layout (location = 0) in vec2 uv;
layout (location = 1) in vec3 fragPos;
layout (location = 2) in vec3 normal;

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler2D colorTex;
layout(set = 1, binding = 1) uniform Material {
    vec3 ambient;
	float pad1;
    vec3 diffuse;
	float pad2;
    vec3 specular;
    float shininess;
} material;

void main() 
{
    vec4 texColor = texture(colorTex, uv);

    // Ambient
    vec3 ambient = sceneData.lightColor * material.ambient;

    // Diffuse
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(sceneData.lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = material.diffuse * diff * sceneData.lightColor;

    // Specular
    vec3 viewDir = normalize(sceneData.cameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = material.specular * spec * sceneData.lightColor;

    outFragColor = vec4((specular + diffuse + ambient), 1.0) * texColor;
}
