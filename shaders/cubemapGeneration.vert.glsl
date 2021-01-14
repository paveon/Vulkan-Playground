#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 LocalPos;
layout(location = 1) out vec2 TexCoords;

layout(push_constant) uniform PushData {
    mat4 PV;
} constants;

void main() {
    LocalPos = inPosition;
    TexCoords = inPosition.yz + 0.5f;
    gl_Position = constants.PV * vec4(LocalPos, 1.0);
}