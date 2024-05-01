layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 ambientColor;
    vec3 cameraPos;
    float pad1;
} sceneData;

struct AttenuationFactors {
    float quadratic, linear, constant;
};

layout(set = 0, binding = 1) uniform PointLight {
    vec3 position;
    float pad1;
    vec3 color;
    float pad2;
    AttenuationFactors attenuationFactors;
} pointLight;

