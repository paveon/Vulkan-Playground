#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 Normal;
layout(location = 1) out vec3 FragPos;
layout(location = 2) out vec2 FragTexCoord;

layout(std430, set=0, binding = 0) uniform TransformUBO {
    mat4 mvp;
    mat4 viewModel;
    mat4 normalMatrix;
} transformUBO;

layout(std430, set=2, binding = 0) uniform ObjectData {
    mat4 mvp;
    mat4 viewModel;
    mat4 normalMatrix;
    vec3 color;
    int materialIdx;
} objectUBO;

void main() {
    gl_Position = objectUBO.mvp * vec4(inPosition, 1.0);
    FragPos = vec3(objectUBO.viewModel * vec4(inPosition, 1.0));
    Normal = normalize(vec3(objectUBO.normalMatrix * vec4(inNormal, 0.0f)));
    FragTexCoord = inTexCoord;
}