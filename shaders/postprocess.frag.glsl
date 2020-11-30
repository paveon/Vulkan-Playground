#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 outUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform FrameData {
    uint frameIndex;
} constants;

layout(set=0, binding = 0) uniform sampler2D texSamplers[];

const float offset = 1.0 / 300.0;

void main() {
//    vec2 offsets[9] = vec2[](
//    vec2(-offset, offset), // top-left
//    vec2(0.0f, offset), // top-center
//    vec2(offset, offset), // top-right
//    vec2(-offset, 0.0f), // center-left
//    vec2(0.0f, 0.0f), // center-center
//    vec2(offset, 0.0f), // center-right
//    vec2(-offset, -offset), // bottom-left
//    vec2(0.0f, -offset), // bottom-center
//    vec2(offset, -offset)// bottom-right
//    );
//
//    float kernel[9] = float[](
//    1, 1, 1,
//    1, -8, 1,
//    1, 1, 1
//    );
//
//    vec3 sampleTex[9];
//    for (int i = 0; i < 9; i++) {
//        sampleTex[i] = vec3(texture(texSamplers[constants.frameIndex], outUV.st + offsets[i]));
//    }
//    vec3 color = vec3(0.0);
//    for (int i = 0; i < 9; i++) {
//        color += sampleTex[i] * kernel[i];
//    }
//
//    outColor = vec4(color, 1.0);

//    vec2 flippedUV = vec2(outUV.x, 1 - outUV.y);
    outColor = vec4(texture(texSamplers[constants.frameIndex], outUV).xyz, 1.0f);
//    outColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
}