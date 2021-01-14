#version 450
#extension GL_EXT_scalar_block_layout : enable

//layout(location = 0) in vec3 inPosition;
//layout(location = 1) in vec3 inNormal;
//layout(location = 2) in vec3 inTangent;
//layout(location = 3) in vec3 inBitangent;
//layout(location = 4) in vec2 inTexCoords;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 TexCoords;

//layout(std430, set=0, binding = 0) uniform TransformUBO {
//    mat4 mvp;
//    mat4 model;
//    mat4 view;
//    mat4 projection;
//    mat4 viewModel;
//    mat4 normalMatrix;
//} transformUBO;

layout(push_constant) uniform PushData {
    mat4 PV;
    uint skyboxTexIdx;
    float lodLevel;
} constants;

void main()
{
    TexCoords = inPosition;
//    TexCoords = vec3(-inPosition.x, inPosition.yz);
//    TexCoords.yz = TexCoords.zy;
    gl_Position = constants.PV * vec4(inPosition, 1.0);
}