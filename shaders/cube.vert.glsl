#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inTexCoords;


layout(location = 0) out vec3 NormalView;
layout(location = 1) out vec3 NormalWorld;
layout(location = 2) out vec3 FragPosView;
layout(location = 3) out vec3 FragPosWorld;
layout(location = 4) out vec2 TexCoords;
layout(location = 5) out flat mat3 TBN;

layout(std430, set=0, binding = 0) uniform TransformUBO {
    mat4 mvp;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 viewModel;
    mat4 viewNormalMatrix;
    mat4 modelNormalMatrix;
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

//    vec3 T = normalize(vec3(transformUBO.viewNormalMatrix * vec4(inTangent, 0.0)));
//    vec3 B = normalize(vec3(transformUBO.viewNormalMatrix * vec4(inBitangent, 0.0)));
//    vec3 N = normalize(vec3(transformUBO.viewNormalMatrix * vec4(inNormal, 0.0)));
    vec3 T = normalize(vec3(transformUBO.modelNormalMatrix * vec4(inTangent, 0.0)));
    vec3 B = normalize(vec3(transformUBO.modelNormalMatrix * vec4(inBitangent, 0.0)));
    vec3 N = normalize(vec3(transformUBO.modelNormalMatrix * vec4(inNormal, 0.0)));
    TBN = mat3(T, B, N);

    gl_Position = transformUBO.mvp * vec4(inPosition, 1.0);
    FragPosView = vec3(transformUBO.viewModel * vec4(inPosition, 1.0));
    FragPosWorld = vec3(transformUBO.model * vec4(inPosition, 1.0));
    NormalView = normalize(vec3(transformUBO.viewNormalMatrix * vec4(inNormal, 0.0f)));
    NormalWorld = normalize(vec3(transformUBO.modelNormalMatrix * vec4(inNormal, 0.0f)));
    TexCoords = inTexCoords;
}