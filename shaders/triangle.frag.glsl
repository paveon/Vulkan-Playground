#version 450
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 FragPos;
layout(location = 2) in vec2 FragTexCoord;

layout(location = 0) out vec4 outColor;

struct DirectionalLight{
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

struct PointLight{
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 coeffs;
};

struct SpotLight {
    vec4 position;
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 cutOffs;
};

layout(std430, set=0, binding = 0) uniform SceneData {
    vec4 cameraPos;
    DirectionalLight light;
    SpotLight spotLight;
    int pointLightCount;
} sceneUBO;

layout(std430, set=1, binding=0) uniform MaterialData {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
    uint diffuseTexIdx;
    uint specularTexIdx;
    uint emissionTexIdx;
} materialUBO;

layout(std430, set=2, binding=0) uniform ObjectData {
    mat4 mvp;
    mat4 viewModel;
    mat4 normalMatrix;
    vec3 color;
    int materialIdx;
} objectUBO;

layout(set=3, binding = 0) uniform sampler2D texSamplers[];

//const uint MAX_LIGHTS = 5;
//layout(std430, set=4, binding=0) uniform LightsUBO {
//    PointLight pointLights[MAX_LIGHTS];
//} lightsUBO;

void main() {
    vec3 norm = normalize(Normal);
    vec3 ambient = vec3(materialUBO.ambient * sceneUBO.light.ambient);

    //    vec3 lightDir = normalize(sceneUBO.light.position.xyz - FragPos);
    vec3 lightDir = normalize(-sceneUBO.light.direction.xyz);
    float diff = max(dot(norm, lightDir), 0.0f);
    vec3 diffuse = vec3((diff * materialUBO.diffuse) * sceneUBO.light.diffuse);

    vec3 eyeDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(eyeDir, reflectDir), 0.0), materialUBO.shininess);
    vec3 specular = vec3((spec * materialUBO.specular) * sceneUBO.light.specular);

    vec3 fragColor = vec3(texture(texSamplers[objectUBO.materialIdx], vec2(FragTexCoord.x, 1 - FragTexCoord.y)));
    outColor = vec4((ambient + diffuse + specular) * fragColor, 1.0f);

    //    outColor = vec4(Normal, 1.0f);
}