#ifndef VULKAN_SHADERPROGRAMVK_H
#define VULKAN_SHADERPROGRAMVK_H


#include <Engine/Renderer/ShaderProgram.h>
#include <Engine/Renderer/vulkan_wrappers.h>


class ShaderProgramVk : public ShaderProgram {
private:
   vk::ShaderModule m_ShaderModule;

public:
   explicit ShaderProgramVk(const char* filepath);

   VkPipelineShaderStageCreateInfo createInfo(VkShaderStageFlagBits stage) const {
      VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
      fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      fragShaderStageInfo.stage = stage;
      fragShaderStageInfo.module = m_ShaderModule.data();
      fragShaderStageInfo.pName = "main";
      return fragShaderStageInfo;
   }
};


#endif //VULKAN_SHADERPROGRAMVK_H
