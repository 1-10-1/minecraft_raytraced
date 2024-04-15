#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 inTangent;
layout(location = 4) in vec2 inBitangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = vec3(0.0, 1.0, 1.0);
    fragTexCoord = inTexCoord;
}

