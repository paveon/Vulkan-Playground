#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 TexCoords;

layout(std430, set=0, binding = 0) uniform TransformUBO {
    mat4 mvp;
    mat4 model;
    mat4 view;
    mat4 viewModel;
    mat4 normalMatrix;
} transformUBO;

void main()
{
    TexCoords = inPosition;
//    TexCoords.xy *= -1.0f;
    gl_Position = transformUBO.mvp * vec4(inPosition, 1.0);
}