#version 460

#extension GL_GOOGLE_include_directive : require

#include "uniforms.glsl"

layout (location = 0) in vec2 uv;
layout (location = 1) in vec3 fragPos;
layout (location = 2) in vec3 normal;

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler2D diffuseTex;
layout(set = 1, binding = 1) uniform sampler2D specularTex;
layout(set = 1, binding = 2) uniform Material {
    float shininess;
} material;

vec3 calcPointLightContribution() {
    float ambientIntensity = 0.2;
    float diffuseIntensity = 1.0;
    float specularIntensity = 1.0;

    float distance    = length(pointLight.position - fragPos);
    float attenuation = 1.0 / (
	pointLight.attenuationFactors.constant
      + pointLight.attenuationFactors.linear    * distance
      + pointLight.attenuationFactors.quadratic * distance * distance
    );

    vec4 matDiffuse = texture(diffuseTex, uv);

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(pointLight.position - fragPos);
    vec3 viewDir = normalize(sceneData.cameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float diff = max(dot(norm, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient = ambientIntensity * matDiffuse.xyz;
    vec3 diffuse = diffuseIntensity * diff * pointLight.color * matDiffuse.xyz;
    vec3 specular = specularIntensity * spec * texture(specularTex, uv).xyz;

    return attenuation * (specular + diffuse + ambient);
}

void main() 
{
    outFragColor = vec4(calcPointLightContribution(), 1.0);
}
