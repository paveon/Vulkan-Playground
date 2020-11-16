#include "ShaderProgram.h"
#include <Platform/Vulkan/ShaderProgramVk.h>

#include "RendererAPI.h"


auto ShaderProgram::Create(const std::vector<std::pair<const char *, ShaderType>> &shaders,
                           const std::vector<std::pair<uint32_t, uint32_t>> &dynamicOverrides) -> std::unique_ptr<ShaderProgram> {
    std::vector<uint32_t> overrides;
    std::transform(dynamicOverrides.begin(), dynamicOverrides.end(), std::back_inserter(overrides),
                   [](const std::pair<uint32_t, uint32_t> &x) { return (x.first << 16u) + x.second; });
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<ShaderProgramVk>(shaders, overrides);
    }

    return nullptr;
}