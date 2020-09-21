#include "Pipeline.h"
#include <Platform/Vulkan/PipelineVk.h>

#include "RendererAPI.h"
#include <Platform/Vulkan/ShaderProgramVk.h>
#include <Platform/Vulkan/RenderPassVk.h>

auto Pipeline::Create(const RenderPass& renderPass,
                 const ShaderProgram& vertexShader,
                 const ShaderProgram& fragShader,
                 const VertexLayout& vertexLayout,
                 const DescriptorLayout& descriptorLayout,
                 const std::vector<PushConstant>& pushConstants,
                 bool enableDepthTest) -> std::unique_ptr<Pipeline> {

   switch (RendererAPI::GetSelectedAPI()) {
       case RendererAPI::API::VULKAN:
         return std::make_unique<PipelineVk>(static_cast<const RenderPassVk&>(renderPass),
                                             static_cast<const ShaderProgramVk&>(vertexShader),
                                             static_cast<const ShaderProgramVk&>(fragShader),
                                             vertexLayout, descriptorLayout, pushConstants, enableDepthTest);
   }

   return nullptr;
}
