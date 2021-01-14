#version 450

layout(location = 0) in vec3 LocalPos;
layout(location = 1) in vec2 TexCoords;

layout(location = 0) out vec4 FragColor;

layout(set=0, binding = 0) uniform samplerCube environmentMap;

const float PI = 3.14159265359f;

void main() {
    vec3 normal = normalize(LocalPos);

    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    if (abs(normal.y) > 0.999) {
        up = normalize(vec3(0.0f, 0.9f, 0.1f));
    }
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    vec3 irradiance = vec3(0.0);
    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    FragColor = vec4(irradiance, 1.0);
}