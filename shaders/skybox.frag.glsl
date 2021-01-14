#version 450
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 TexCoords;

layout(location = 0) out vec4 outColor;

//layout(set=1, binding = 0) uniform samplerCube cubeSamplers[];
layout(set=0, binding = 0) uniform samplerCube cubeSamplers[];

//layout(std430, set=1, binding=1) uniform MaterialData {
//    int skyboxTexIdx;
//} materialUBO;

layout(push_constant) uniform PushData {
    mat4 PV;
    uint skyboxTexIdx;
    float lodLevel;
} constants;


void main() {
//    outColor = vec4(1.0f);
    outColor = vec4(textureLod(cubeSamplers[constants.skyboxTexIdx], TexCoords, constants.lodLevel).rgb, 1.0f);
//    outColor = vec4(textureLod(cubeSamplers[0], TexCoords, 1.8f).rgb, 1.0f);
}