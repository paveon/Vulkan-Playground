#version 450

layout(location = 0) in vec3 LocalPos;
layout(location = 1) in vec2 TexCoords;

layout(location = 0) out vec4 FragColor;

layout(push_constant) uniform PushData {
    mat4 PV;
    float roughness;
    uint baseResolution;
} constants;

layout(set=0, binding = 0) uniform samplerCube environmentMap;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;// / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = normalize(cross(N, tangent));

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}


float DistributionGGX(float NdotH, float roughness)
{
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = a * a;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return num / denom;
}


void main()
{
    vec3 N = normalize(LocalPos);
    vec3 R = N;
    vec3 V = R;

    float resolution = constants.baseResolution;
    float saTexel  = 4.0f * PI / (6.0f * resolution * resolution);

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;
    vec3 prefilteredColor = vec3(0.0);
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, constants.roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotH = max(dot(N, H), 0.0f);
        float HdotV = max(dot(H, V), 0.0f);

        float D   = DistributionGGX(NdotH, constants.roughness);
        float pdf = (D * NdotH / (4.0f * HdotV)) + 0.0001f;
        float saSample = 1.0f / (float(SAMPLE_COUNT) * pdf + 0.0001f);
        float mipLevel = constants.roughness == 0.0f ? 0.0f : 0.5f * log2(saSample / saTexel);

        float NdotL = max(dot(N, L), 0.0f);
        if (NdotL > 0.0f)
        {
            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

    FragColor = vec4(prefilteredColor, 1.0f);
}