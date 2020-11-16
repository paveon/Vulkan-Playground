#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 Normal;

layout(location = 0) out vec4 outColor;

//struct DirectionalLight{
//    vec4 direction;
//    vec4 ambient;
//    vec4 diffuse;
//    vec4 specular;
//};
//
//struct PointLight{
//    vec4 position;
//    vec4 ambient;
//    vec4 diffuse;
//    vec4 specular;
//    vec4 coeffs;
//};
//
//struct SpotLight {
//    vec4  position;
//    vec4  direction;
//    vec4 ambient;
//    vec4 diffuse;
//    vec4 specular;
//    vec4 cutOffs;
//};

//layout(std430, set=0, binding = 0) uniform SceneData {
//    vec4 cameraPos;
//    DirectionalLight light;
//    SpotLight spotLight;
//    int pointLightCount;
//} sceneUBO;

void main() {
    outColor = vec4(1.0f);
}