layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 ambientColor;
    vec3 lightPos;
    float pad1;
    vec3 lightColor;
    float pad2;
    vec3 cameraPos;
} sceneData;

