#ifndef GAME_ENGINE_SHADER_RESOURCES_H
#define GAME_ENGINE_SHADER_RESOURCES_H

#include <string>
#include <cstdint>
#include <unordered_map>

struct UniformMember {
    uint32_t offset;
    uint32_t size;
};

struct UniformBinding {
    std::string name;
    uint32_t size;
    uint32_t count;
    bool perObject;
    std::unordered_map<std::string, UniformMember> members;
};

struct SamplerBinding {
    enum class Type {
        SAMPLER_1D,
        SAMPLER_2D,
        SAMPLER_3D,
        SAMPLER_CUBE
    };

    std::string name;
    uint32_t count;
    Type type;
    bool array;
};


#endif //GAME_ENGINE_SHADER_RESOURCES_H
