#version 450
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : require

const float PI = 3.14159265359;

layout(location = 0) in vec3 NormalView;
layout(location = 1) in vec3 NormalWorld;
layout(location = 2) in vec3 FragPosView;
layout(location = 3) in vec3 FragPosWorld;
layout(location = 4) in vec2 TexCoords;
layout(location = 5) in mat3 TBN;

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

layout(set=1, binding = 0) uniform sampler2D texSamplers[];
layout(set=2, binding = 0) uniform samplerCube cubeSamplers[];


layout(std430, set=3, binding = 0) uniform SceneData {
    vec4 cameraPos;
    DirectionalLight light;
    SpotLight spotLight;
    int pointLightCount;
} sceneUBO;

const uint MAX_LIGHTS = 5;
layout(std430, set=4, binding=0) uniform LightsUBO {
    PointLight pointLights[MAX_LIGHTS];
} lightsUBO;


layout(std430, set=5, binding=0) uniform MaterialData {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 albedo;
    float shininess;
    float metallic;
    float roughness;
    float ao;
    int diffuseTexIdx;
    int specularTexIdx;
    int emissionTexIdx;

    int environmentMapTexIdx;
    int irradianceMapTexIdx;
    int prefilterMapTexIdx;
    int brdfLutIdx;

    int normalMapTexIdx;
    int albedoMapTexIdx;
    int metallicMapTexIdx;
    int roughnessMapTexIdx;
    int aoMapTexIdx;

    int enableNormalTex;
    int enableAlbedoTex;
    int enableMetallicTex;
    int enableRoughnessTex;
    int enableAoTex;
} materialUBO;


layout(std430, set=0, binding = 0) uniform TransformUBO {
    mat4 mvp;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 viewModel;
    mat4 viewNormalMatrix;
    mat4 modelNormalMatrix;
} transformUBO;


float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH*NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return num / denom;
}


float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r*r) / 8.0f;

    float num   = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return num / denom;
}


float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}


vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}


vec3 CalcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec4 diffTexel, vec4 specTexel) {
    vec3 lightDirView = normalize(light.direction.xyz);
    //    vec3 lightDir = normalize(-light.direction.xyz);
    vec3 reflectDir = reflect(lightDirView, normal);
    vec3 halfwayDir = normalize((-lightDirView) + viewDir);

    float diff = max(dot(normal, -lightDirView), 0.0f);
    //    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialUBO.shininess);
    float spec = diff > 0.0f ? pow(max(dot(normal, halfwayDir), 0.0), materialUBO.shininess) : 0.0f;

    vec3 ambient = vec3(diffTexel * light.ambient);
    vec3 diffuse = vec3(diffTexel * light.diffuse * diff);
    vec3 specular = vec3(specTexel * light.specular * spec);
    return (ambient + diffuse + specular);
}


vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec4 diffTexel, vec4 specTexel) {
    float dist = distance(light.position.xyz, fragPos);
    float attenuation = 1.0f / dot(vec4(1.0f, dist, dist * dist, 0.0f), light.coeffs);

    vec3 pointLightDir = normalize(light.position.xyz - fragPos);
    vec3 reflectPointDir = reflect(-pointLightDir, normal);

    float diff = max(dot(normal, pointLightDir), 0.0f);
    float spec = pow(max(dot(viewDir, reflectPointDir), 0.0), materialUBO.shininess);

    vec3 ambient = vec3(diffTexel * light.ambient) * attenuation;
    vec3 diffuse = vec3(diffTexel * light.diffuse * diff) * attenuation;
    vec3 specular = vec3(specTexel * light.specular * spec) * attenuation;
    return (ambient + diffuse + specular);
}


vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec4 diffTexel, vec4 specTexel) {
    vec3 lightDir = normalize(light.position.xyz - fragPos);
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


const float MAX_REFLECTION_LOD = 4.0;


void main() {
    vec2 flippedTexCoords = vec2(TexCoords.x, 1 - TexCoords.y);

//    vec4 diffuseTexel = materialUBO.diffuseTexIdx >= 0 ? texture(texSamplers[materialUBO.diffuseTexIdx], TexCoords) : vec4(0.0f);
//    vec4 specularTexel = materialUBO.specularTexIdx >= 0 ? texture(texSamplers[materialUBO.specularTexIdx], TexCoords) : vec4(vec3(0.5f), 1.0f);
    vec3 albedo = materialUBO.albedoMapTexIdx >= 0 && materialUBO.enableAlbedoTex == 1 ?
        texture(texSamplers[materialUBO.albedoMapTexIdx], TexCoords).rgb : materialUBO.albedo.rgb;

    float metallic = materialUBO.metallicMapTexIdx >= 0 && materialUBO.enableMetallicTex == 1 ?
        texture(texSamplers[materialUBO.metallicMapTexIdx], TexCoords).r : materialUBO.metallic;

    float roughness = materialUBO.roughnessMapTexIdx >= 0 && materialUBO.enableRoughnessTex == 1 ?
        texture(texSamplers[materialUBO.roughnessMapTexIdx], TexCoords).r : materialUBO.roughness;

    float ao = materialUBO.aoMapTexIdx >= 0 && materialUBO.enableAoTex == 1 ?
        texture(texSamplers[materialUBO.aoMapTexIdx], TexCoords).r : materialUBO.ao;

    vec3 normal;
    if (materialUBO.normalMapTexIdx >= 0 && materialUBO.enableNormalTex == 1) {
        normal = texture(texSamplers[materialUBO.normalMapTexIdx], TexCoords).rgb;
        normal = normal * 2.0 - 1.0;
        normal = normalize(TBN * normal);
        //        normal = normalize(vec3(transformUBO.viewNormalMatrix * vec4(normal, 0.0f)));
    } else {
        normal = normalize(NormalWorld);
    }

    vec3 eyeDir = normalize(-FragPosView);

    //    vec3 result = CalcDirLight(sceneUBO.light, normal, eyeDir, diffuseTexel, specularTexel);
    //    result += CalcSpotLight(sceneUBO.spotLight, normal, FragPosView, eyeDir, diffuseTexel, specularTexel);
    //    vec3 result = vec3(0.0f);

    vec3 N = normalize(normal);
    vec3 V = normalize(sceneUBO.cameraPos.xyz - FragPosWorld);
    vec3 R = reflect(-V, N);

    float NdotV = max(dot(N, V), 0.0f);

    vec3 F0 = mix(vec3(0.04f), albedo, metallic);

    vec3 Lo = vec3(0.0f);
    for (int i = 0; i < min(MAX_LIGHTS, sceneUBO.pointLightCount); i++) {
        PointLight pointLight = lightsUBO.pointLights[i];
        vec3 toLight = pointLight.position.xyz - FragPosWorld;
        vec3 L = normalize(toLight);
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0f);

        // Per-light radiance
        float distanceSq = dot(toLight, toLight);
        float attenuation = 1.0f / distanceSq;
        vec3 radiance = pointLight.diffuse.rgb * attenuation;//TODO: should be just light color?

        // Cook-Torrance BRDF
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0f * NdotV * NdotL;
        vec3 specular = numerator / max(denominator, 0.001f);

        vec3 kS = F;
        vec3 kD = (vec3(1.0f) - kS) * (1.0f - metallic);


        Lo += (kD * (albedo / PI) + specular) * radiance * NdotL;
        //        result += CalcPointLight(lightsUBO.pointLights[i], normal, FragPosView, eyeDir, diffuseTexel, specularTexel);
    }


    vec3 prefilteredTexel = vec3(0.0f);
    int prefilterIdx = materialUBO.prefilterMapTexIdx;
    if (prefilterIdx >= 0) {
        prefilteredTexel = textureLod(cubeSamplers[prefilterIdx], R,  roughness * MAX_REFLECTION_LOD).rgb;
    }

    vec3 irradianceTexel = vec3(0.0f);
    int irradianceIdx = materialUBO.irradianceMapTexIdx;
    if (irradianceIdx >= 0) {
        irradianceTexel = texture(cubeSamplers[irradianceIdx], N).rgb;
    }

    vec2 envBRDF = vec2(0.0f);
    int brdfIdx = materialUBO.brdfLutIdx;
    if (brdfIdx >= 0) {
        envBRDF = texture(texSamplers[brdfIdx], vec2(NdotV, roughness)).rg;
    }

    vec3 kS = fresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;

    vec3 diffuse = irradianceTexel * albedo;
    vec3 specular = prefilteredTexel * (kS * envBRDF.x + envBRDF.y);
    vec3 ambient = (kD * diffuse + specular) * ao;

    vec3 color = ambient + Lo;
    outColor = vec4(color, 1.0f);

//    outColor = vec4(normal, 1.0f);


//    vec3 ambient = vec3(0.0f);
//    if (materialUBO.irradianceMapTexIdx >= 0) {
//        vec3 irradianceTexel = texture(cubeSamplers[materialUBO.irradianceMapTexIdx], N).rgb;
//        vec3 kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
//        vec3 kD = 1.0 - kS;
//        vec3 diffuse = irradianceTexel * albedo;
//        ambient = (kD * diffuse) * ao;
//    } else {
//        ambient = vec3(0.03f) * albedo * ao;
//    }

//    vec3 color = ambient + Lo;
//
//    outColor = vec4(color, 1.0f);

    //    outColor = vec4(result, 1.0f);

    //    vec3 I = normalize(FragPosWorld - vec3(sceneUBO.cameraPos));
    //    vec3 R = reflect(I, normalize(NormalWorld));
    //    outColor = vec4(texture(cubeSamplers[materialUBO.environmentTexIdx], R).rgb, 1.0f);

    //    outColor = vec4(diffuseTexel.xyz, 1.0f);
    //    float depth = LinearizeDepth(gl_FragCoord.z) / far;
    //    outColor = vec4(vec3(depth), 1.0);
    //    outColor = vec4(normal, 1.0f);
}