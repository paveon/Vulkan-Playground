#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inTexCoords;

layout(location = 0) out vec3 outNormal;

void main() {
//    gl_Position = transformUBO.mvp * vec4(inPosition * 1.1f, 1.0);
//    outNormal = inNormal;

    outNormal = inNormal;
    gl_Position = vec4(inPosition, 1.0f);

//    gl_Position = transformUBO.viewModel * vec4(inPosition, 1.0);
//    NormalView = normalize(vec3(transformUBO.viewNormalMatrix * vec4(inNormal, 0.0f)));
}