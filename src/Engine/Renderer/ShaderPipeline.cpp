#include "ShaderPipeline.h"
#include <Platform/Vulkan/ShaderPipelineVk.h>

#include "RendererAPI.h"


auto ShaderPipeline::Create(std::string name,
                            const std::vector<std::pair<const char *, ShaderType>> &shaders,
                            const std::vector<BindingKey> &perObjectUniforms,
                            const RenderPass &renderPass,
                            const std::vector<PushConstant> &pushConstants,
                            bool enableDepthTest) -> std::unique_ptr<ShaderPipeline> {
//    std::vector<uint32_t> dynamics;
//    std::transform(perObjectUniforms.begin(), perObjectUniforms.end(), std::back_inserter(dynamics),
//                   [](const std::pair<uint32_t, uint32_t> &x) { return (x.first << 16u) + x.second; });

    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<ShaderPipelineVk>(std::move(name),
                                                      shaders,
                                                      perObjectUniforms,
                                                      renderPass,
                                                      pushConstants,
                                                      enableDepthTest);
    }

    return nullptr;
}