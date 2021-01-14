#include "ShaderPipeline.h"
#include <Platform/Vulkan/ShaderPipelineVk.h>

#include "RendererAPI.h"


auto ShaderPipeline::Create(std::string name,
                            const std::map<ShaderType, const char *> &shaders,
                            const std::unordered_set<BindingKey> &perObjectUniforms,
                            const RenderPass &renderPass,
                            uint32_t subpassIndex,
                            VkPrimitiveTopology topology,
                            std::pair<VkCullModeFlags, VkFrontFace> culling,
                            DepthState depthState,
                            MultisampleState msState) -> std::unique_ptr<ShaderPipeline> {

    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<ShaderPipelineVk>(std::move(name),
                                                      shaders,
                                                      perObjectUniforms,
                                                      static_cast<const RenderPassVk &>(renderPass),
                                                      subpassIndex,
                                                      topology,
                                                      culling,
                                                      depthState,
                                                      msState);
    }

    return nullptr;
}