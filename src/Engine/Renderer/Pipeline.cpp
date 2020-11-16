#include <Platform/Vulkan/PipelineVk.h>

#include "RendererAPI.h"

auto Pipeline::Create(const RenderPass& renderPass,
                 const ShaderProgram& program,
                 const std::vector<PushConstant>& pushConstants,
                 bool enableDepthTest) -> std::unique_ptr<Pipeline> {

   switch (RendererAPI::GetSelectedAPI()) {
       case RendererAPI::API::VULKAN:
         return std::make_unique<PipelineVk>(renderPass,
                                             program,
                                             pushConstants,
                                             enableDepthTest);
   }

   return nullptr;
}
