#version 450
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : require

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (location = 0) in vec3 inNormal[];

layout (location = 0) out vec3 outColor;

layout(std430, set=0, binding = 0) uniform TransformUBO {
    mat4 mvp;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 viewModel;
    mat4 viewNormalMatrix;
    mat4 modelNormalMatrix;
} transformUBO;

const float normalLength = 0.05;

void main(void) {
    vec3 pos = gl_in[0].gl_Position.xyz;
    vec3 normal = inNormal[0].xyz;

    gl_Position = transformUBO.mvp * vec4(pos, 1.0);
    outColor = vec3(1.0, 0.0, 0.0);
    EmitVertex();

    gl_Position = transformUBO.mvp * vec4(pos + normal * normalLength, 1.0);
    outColor = vec3(0.0, 0.0, 1.0);
    EmitVertex();

    EndPrimitive();


    pos = gl_in[1].gl_Position.xyz;
    normal = inNormal[1].xyz;

    gl_Position = transformUBO.mvp * vec4(pos, 1.0);
    outColor = vec3(1.0, 0.0, 0.0);
    EmitVertex();

    gl_Position = transformUBO.mvp * vec4(pos + normal * normalLength, 1.0);
    outColor = vec3(0.0, 0.0, 1.0);
    EmitVertex();

    EndPrimitive();

    pos = gl_in[2].gl_Position.xyz;
    normal = inNormal[2].xyz;

    gl_Position = transformUBO.mvp * vec4(pos, 1.0);
    outColor = vec3(1.0, 0.0, 0.0);
    EmitVertex();

    gl_Position = transformUBO.mvp * vec4(pos + normal * normalLength, 1.0);
    outColor = vec3(0.0, 0.0, 1.0);
    EmitVertex();

    EndPrimitive();
}