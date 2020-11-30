#include "ShaderPipeline.h"
#include <Platform/Vulkan/ShaderPipelineVk.h>

#include "RendererAPI.h"


auto ShaderPipeline::Create(std::string name,
                            const std::vector<std::pair<const char *, ShaderType>> &shaders,
                            const std::unordered_set<BindingKey> &perObjectUniforms,
                            const RenderPass &renderPass,
                            uint32_t subpassIndex,
                            std::pair<VkCullModeFlags, VkFrontFace> culling,
                            DepthState depthState) -> std::unique_ptr<ShaderPipeline> {

    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<ShaderPipelineVk>(std::move(name),
                                                      shaders,
                                                      perObjectUniforms,
                                                      renderPass,
                                                      subpassIndex,
                                                      culling,
                                                      depthState);
    }

    return nullptr;
}