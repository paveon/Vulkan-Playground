#version 450
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 FragPos;
layout(location = 2) in vec2 TexCoords;

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
    vec4  direction;
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

layout(set=3, binding = 0) uniform sampler2D texSamplers[];


const uint MAX_LIGHTS = 5;
layout(std430, set=4, binding=0) uniform LightsUBO {
    PointLight pointLights[MAX_LIGHTS];
} lightsUBO;


vec3 CalcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec4 diffTexel, vec4 specTexel) {
    vec3 lightDir = normalize(-light.direction.xyz);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0f);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialUBO.shininess);

    vec3 ambient = vec3(diffTexel * light.ambient);
    vec3 diffuse = vec3(diffTexel * light.diffuse * diff);
    vec3 specular = vec3(specTexel * light.specular * spec);
    return (ambient + diffuse + specular);
}


vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec4 diffTexel, vec4 specTexel) {
    float dist = distance(light.position.xyz, FragPos);
    float attenuation = 1.0f / dot(vec4(1.0f, dist, dist * dist, 0.0f), light.coeffs);

    vec3 pointLightDir = normalize(light.position.xyz - FragPos);
    vec3 reflectPointDir = reflect(-pointLightDir, normal);

    float diff = max(dot(normal, pointLightDir), 0.0f);
    float spec = pow(max(dot(viewDir, reflectPointDir), 0.0), materialUBO.shininess);

    vec3 ambient = vec3(diffTexel * light.ambient) * attenuation;
    vec3 diffuse = vec3(diffTexel * light.diffuse * diff) * attenuation;
    vec3 specular = vec3(specTexel * light.specular * spec) * attenuation;
    return (ambient + diffuse + specular);
}


vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec4 diffTexel, vec4 specTexel) {
    vec3 lightDir = normalize(light.position.xyz - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    float theta = dot(lightDir, normalize(-light.direction.xyz));
    float epsilon = light.cutOffs[0] - light.cutOffs[1];
    float intensity = clamp((theta - light.cutOffs[1]) / epsilon, 0.0, 1.0);

    float diff = max(dot(normal, lightDir), 0.0f);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialUBO.shininess);

    vec3 ambient = vec3(diffTexel * light.ambient);
    vec3 diffuse = vec3(diffTexel * light.diffuse * diff) * intensity;
    vec3 specular = vec3(specTexel * light.specular * spec) * intensity;
    return (ambient + diffuse + specular);
}


void main() {
//    vec2 flippedTexCoords = vec2(TexCoords.x, 1 - TexCoords.y);

    vec4 diffuseTexel = texture(texSamplers[materialUBO.diffuseTexIdx], TexCoords);
    vec4 specularTexel = texture(texSamplers[materialUBO.specularTexIdx], TexCoords);
    vec3 norm = normalize(Normal);
    vec3 eyeDir = normalize(-FragPos);

    vec3 result = CalcDirLight(sceneUBO.light, norm, eyeDir, diffuseTexel, specularTexel);
    result += CalcSpotLight(sceneUBO.spotLight, norm, FragPos, eyeDir, diffuseTexel, specularTexel);

    for (int i = 0; i < min(MAX_LIGHTS, sceneUBO.pointLightCount); i++) {
        result += CalcPointLight(lightsUBO.pointLights[i], norm, FragPos, eyeDir, diffuseTexel, specularTexel);
    }

//    outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    outColor = vec4(result, 1.0f);
}