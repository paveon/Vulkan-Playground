#ifndef VULKAN_PIPELINEVK_H
#define VULKAN_PIPELINEVK_H


#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/vulkan_wrappers.h>
#include <Engine/Application.h>
#include "TextureVk.h"

class ShaderProgramVk;

class RenderPassVk;

class PipelineVk : public Pipeline {
private:
    vk::PipelineCache m_Cache;
    vk::DescriptorPool m_DescriptorPool;
    vk::DescriptorSetLayout m_DescriptorSetLayout;
    vk::DescriptorSets m_DescriptorSets;
    vk::Sampler m_Sampler;

    vk::PipelineLayout m_PipelineLayout;
    vk::Pipeline m_Pipeline;

public:
    PipelineVk(const RenderPassVk& renderPass, const ShaderProgramVk& vertexShader,
               const ShaderProgramVk& fragShader, const VertexLayout& vertexLayout,
               const DescriptorLayout& descriptorLayout, const std::vector<PushConstant>& pushConstants,
               bool enableDepthTest);

    void Bind(VkCommandBuffer cmdBuffer, uint32_t frameIdx) const override;

    void BindTexture(const vk::ImageView& texture, uint32_t binding) const {
       VkDescriptorImageInfo fontDescriptor = {};
       fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
       fontDescriptor.imageView = texture.data();
       fontDescriptor.sampler = m_Sampler.data();

       std::vector<VkWriteDescriptorSet> writeDescriptorSets(m_DescriptorSets.size());
       for (uint32_t i = 0; i < m_DescriptorSets.size(); i++) {
          writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writeDescriptorSets[i].dstSet = m_DescriptorSets[i];
          writeDescriptorSets[i].dstBinding = binding;
          writeDescriptorSets[i].dstArrayElement = 0;
          writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
          writeDescriptorSets[i].descriptorCount = 1;
          writeDescriptorSets[i].pImageInfo = &fontDescriptor;
       }

       auto& ctx = static_cast<const GfxContextVk&>(Application::GetGraphicsContext());
       vkUpdateDescriptorSets(ctx.GetDevice(), writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }

    void BindTexture(const Texture2D& texture, uint32_t binding) const override {
       auto& textureVk = static_cast<const Texture2DVk&>(texture);

       VkDescriptorImageInfo fontDescriptor = {};
       fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
       fontDescriptor.imageView = textureVk.View().data();
       fontDescriptor.sampler = m_Sampler.data();

       std::vector<VkWriteDescriptorSet> writeDescriptorSets(m_DescriptorSets.size());
       for (uint32_t i = 0; i < m_DescriptorSets.size(); i++) {
          writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writeDescriptorSets[i].dstSet = m_DescriptorSets[i];
          writeDescriptorSets[i].dstBinding = binding;
          writeDescriptorSets[i].dstArrayElement = 0;
          writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
          writeDescriptorSets[i].descriptorCount = 1;
          writeDescriptorSets[i].pImageInfo = &fontDescriptor;
       }

       auto& ctx = static_cast<const GfxContextVk&>(Application::GetGraphicsContext());
       vkUpdateDescriptorSets(ctx.GetDevice(), writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }

    void BindUniformBuffers(const std::vector<vk::Buffer>& buffers, uint32_t binding) const override {
       std::vector<VkDescriptorBufferInfo> bufferInfo(m_DescriptorSets.size());
       std::vector<VkWriteDescriptorSet> descriptorWrites(m_DescriptorSets.size());
       for (size_t i = 0; i < m_DescriptorSets.size(); i++) {
          bufferInfo[i].buffer = buffers[i].data();
          bufferInfo[i].offset = 0;
          bufferInfo[i].range = buffers[i].Size();

          descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          descriptorWrites[i].dstSet = m_DescriptorSets[i];
          descriptorWrites[i].dstBinding = binding;
          descriptorWrites[i].dstArrayElement = 0;
          descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
          descriptorWrites[i].descriptorCount = 1;
          descriptorWrites[i].pBufferInfo = &bufferInfo[i];
       }

       auto& ctx = static_cast<const GfxContextVk&>(Application::GetGraphicsContext());
       vkUpdateDescriptorSets(ctx.GetDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }

    void PushConstants(VkCommandBuffer cmdBuffer, uint32_t offset, uint32_t size, const void* data) const override {
       vkCmdPushConstants(cmdBuffer, m_PipelineLayout.data(), VK_SHADER_STAGE_VERTEX_BIT, offset, size, data);
    }
};


#endif //VULKAN_PIPELINEVK_H
