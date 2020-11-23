#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 Normal;
layout(location = 1) out vec3 FragPos;
layout(location = 2) out vec2 TexCoords;

layout(std430, set=0, binding = 0) uniform TransformUBO {
    mat4 mvp;
    mat4 model;
    mat4 view;
    mat4 viewModel;
    mat4 normalMatrix;
} transformUBO;

void main() {
//    gl_Position = projection * view * model * vec4(aPos, 1.0);
//    FragPos = vec3(view * model * vec4(aPos, 1.0));
//    Normal = mat3(transpose(inverse(view * model))) * aNormal;
//    LightPos = vec3(view * vec4(lightPos, 1.0)); // Transform world-space light position to view-space light position

//    mat4 normalMatrix = transpose(inverse(transformUBO.model));
//    Normal = normalize(vec3(normalMatrix * vec4(inNormal, 0.0f)));

//    gl_Position = transformUBO.mvp * vec4(inPosition, 1.0);
//    FragPos = vec3(transformUBO.view * transformUBO.model * vec4(inPosition, 1.0));
//    mat4 normalMatrix = transpose(inverse(transformUBO.view * transformUBO.model));
//    Normal = normalize(vec3(normalMatrix * vec4(inNormal, 0.0f)));
//    TexCoords = inTexCoords;

    gl_Position = transformUBO.mvp * vec4(inPosition, 1.0);
    FragPos = vec3(transformUBO.viewModel * vec4(inPosition, 1.0));
    Normal = normalize(vec3(transformUBO.normalMatrix * vec4(inNormal, 0.0f)));
    TexCoords = inTexCoords;
}