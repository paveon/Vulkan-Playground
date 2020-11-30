#version 450
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 TexCoords;

layout(location = 0) out vec4 outColor;

layout(set=1, binding = 0) uniform samplerCube cubeSamplers[];

layout(std430, set=1, binding=1) uniform MaterialData {
    int skyboxTexIdx;
} materialUBO;

void main() {
//    outColor = vec4(1.0f);
    outColor = texture(cubeSamplers[0], TexCoords);
}