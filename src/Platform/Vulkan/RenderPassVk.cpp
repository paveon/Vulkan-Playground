#include "RenderPassVk.h"

#include <vector>
#include <Engine/Application.h>


RenderPassVk::~RenderPassVk() {
   if (m_RenderPass) vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
}


RenderPassVk::RenderPassVk(bool test) :
        m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
        m_Device(m_Context.GetDevice()) {

   auto sampleCount = m_Device.maxSamples();
   auto imageFormat = m_Context.Swapchain().ImageFormat();

   /// TODO: this is atrocious, figure out, how to abstract renderpasses
   /// cleanly to make them possibly usable at application side? Dunno yet
   if (test) {
      /// FIRST PASS
      m_Attachments = {VkAttachmentDescription{
              0,
              VK_FORMAT_R32G32B32A32_SFLOAT,
              VK_SAMPLE_COUNT_1_BIT,
              VK_ATTACHMENT_LOAD_OP_DONT_CARE,
//              VK_ATTACHMENT_LOAD_OP_CLEAR,
              VK_ATTACHMENT_STORE_OP_STORE,
              VK_ATTACHMENT_LOAD_OP_DONT_CARE,
              VK_ATTACHMENT_STORE_OP_DONT_CARE,
              VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      }, VkAttachmentDescription{
              0,
              m_Device.findDepthFormat(),
              sampleCount,
              VK_ATTACHMENT_LOAD_OP_CLEAR,
              VK_ATTACHMENT_STORE_OP_DONT_CARE,
              VK_ATTACHMENT_LOAD_OP_DONT_CARE,
              VK_ATTACHMENT_STORE_OP_DONT_CARE,
              VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      }, VkAttachmentDescription{
              0,
              VK_FORMAT_R32G32B32A32_SFLOAT,
              sampleCount,
              VK_ATTACHMENT_LOAD_OP_CLEAR,
              VK_ATTACHMENT_STORE_OP_STORE,
              VK_ATTACHMENT_LOAD_OP_DONT_CARE,
              VK_ATTACHMENT_STORE_OP_DONT_CARE,
              VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      }};

      m_ColorAttachments = {VkAttachmentReference{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

      VkAttachmentReference resolveAttachmentRef = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
      VkAttachmentReference depthAttachmentRef = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};


      m_Subpasses = {VkSubpassDescription{
              0,
              VK_PIPELINE_BIND_POINT_GRAPHICS,
              0,
              nullptr,
              static_cast<uint32_t>(m_ColorAttachments.size()),
              m_ColorAttachments.data(),
              &resolveAttachmentRef,
              &depthAttachmentRef,
              0,
              nullptr
      }};

      std::vector<VkSubpassDependency> dependencies(2);
      dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[0].dstSubpass = 0;
      dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[0].dependencyFlags = 0;

      dependencies[1].srcSubpass = 0;
      dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependencies[1].dependencyFlags = 0;

      m_ClearValues.resize(m_Attachments.size());

      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = m_Attachments.size();
      renderPassInfo.pAttachments = m_Attachments.data();
      renderPassInfo.subpassCount = m_Subpasses.size();
      renderPassInfo.pSubpasses = m_Subpasses.data();
      renderPassInfo.dependencyCount = dependencies.size();
      renderPassInfo.pDependencies = dependencies.data();

      if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
         throw std::runtime_error("failed to create render pass!");
   } else {
      /// SECOND PASS
      m_Attachments = {VkAttachmentDescription{
              0,
              imageFormat,
              VK_SAMPLE_COUNT_1_BIT,
              VK_ATTACHMENT_LOAD_OP_DONT_CARE,
              VK_ATTACHMENT_STORE_OP_STORE,
              VK_ATTACHMENT_LOAD_OP_DONT_CARE,
              VK_ATTACHMENT_STORE_OP_DONT_CARE,
              VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      }};

      m_ColorAttachments = {VkAttachmentReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

      m_Subpasses = {VkSubpassDescription{
              0,
              VK_PIPELINE_BIND_POINT_GRAPHICS,
              0,
              nullptr,
              static_cast<uint32_t>(m_ColorAttachments.size()),
              m_ColorAttachments.data(),
              nullptr,
              nullptr,
              0,
              nullptr
      }};

      std::vector<VkSubpassDependency> dependencies(2);
      dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[0].dstSubpass = 0;
      dependencies[0].srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[0].dependencyFlags = 0;

      dependencies[1].srcSubpass = 0;
      dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[1].dstAccessMask = 0;
      dependencies[1].dependencyFlags = 0;

      m_ClearValues.resize(m_Attachments.size());

      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = m_Attachments.size();
      renderPassInfo.pAttachments = m_Attachments.data();
      renderPassInfo.subpassCount = m_Subpasses.size();
      renderPassInfo.pSubpasses = m_Subpasses.data();
      renderPassInfo.dependencyCount = dependencies.size();
      renderPassInfo.pDependencies = dependencies.data();

      if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
         throw std::runtime_error("failed to create render pass!");
   }
}


RenderPassVk::RenderPassVk(std::vector<VkAttachmentDescription> attachments,
                           std::vector<VkSubpassDescription> subpasses,
                           std::vector<VkSubpassDependency> dependencies,
                           const std::vector<uint32_t> &colorAttachmentIndices,
                           std::optional<uint32_t> depthAttachmentIndex) :
        m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
        m_Device(m_Context.GetDevice()),
        m_Attachments(std::move(attachments)) {

   for (uint32_t colorAttachmentIdx: colorAttachmentIndices) {
      m_ColorAttachments.push_back(VkAttachmentReference{
              colorAttachmentIdx,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      });
   }

   VkAttachmentReference depthAttachmentRef = {
           depthAttachmentIndex.value_or(0),
           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
   };

//    m_ColorAttachments = {VkAttachmentReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
//    VkAttachmentReference depthAttachmentRef = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

//    m_Attachments = {VkAttachmentDescription{
//            0,
//            VK_FORMAT_R32G32B32A32_SFLOAT,
//            VK_SAMPLE_COUNT_1_BIT,
//            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
//            VK_ATTACHMENT_STORE_OP_STORE,
//            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
//            VK_ATTACHMENT_STORE_OP_DONT_CARE,
//            VK_IMAGE_LAYOUT_UNDEFINED,
//            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//    }, VkAttachmentDescription{
//            0,
//            m_Device.findDepthFormat(),
//            VK_SAMPLE_COUNT_1_BIT,
//            VK_ATTACHMENT_LOAD_OP_CLEAR,
//            VK_ATTACHMENT_STORE_OP_DONT_CARE,
//            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
//            VK_ATTACHMENT_STORE_OP_DONT_CARE,
//            VK_IMAGE_LAYOUT_UNDEFINED,
//            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
//    }};

   if (subpasses.empty()) {
      m_Subpasses = {VkSubpassDescription{
              0,
              VK_PIPELINE_BIND_POINT_GRAPHICS,
              0,
              nullptr,
              static_cast<uint32_t>(m_ColorAttachments.size()),
              m_ColorAttachments.data(),
              nullptr,
              depthAttachmentIndex.has_value() ? &depthAttachmentRef : nullptr,
              0,
              nullptr
      }};
   } else {
      m_Subpasses = subpasses;
   }

   if (dependencies.empty()) {
      dependencies = {
              VkSubpassDependency{
                      VK_SUBPASS_EXTERNAL,
                      0,
                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                      VK_ACCESS_SHADER_READ_BIT,
                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                      0
              },
              VkSubpassDependency{
                      0,
                      VK_SUBPASS_EXTERNAL,
                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                      VK_ACCESS_SHADER_READ_BIT,
                      0
              }
      };
   }

   m_ClearValues.resize(m_Attachments.size());

   VkRenderPassCreateInfo renderPassInfo = {};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderPassInfo.attachmentCount = m_Attachments.size();
   renderPassInfo.pAttachments = m_Attachments.data();
   renderPassInfo.subpassCount = m_Subpasses.size();
   renderPassInfo.pSubpasses = m_Subpasses.data();
   renderPassInfo.dependencyCount = dependencies.size();
   renderPassInfo.pDependencies = dependencies.data();

   if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
      throw std::runtime_error("failed to create render pass!");
}


void RenderPassVk::Begin(const vk::CommandBuffer &cmdBuffer, const vk::Framebuffer &framebuffer) const {
   VkRenderPassBeginInfo renderPassBeginInfo = {};
   renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassBeginInfo.renderPass = m_RenderPass;
   renderPassBeginInfo.clearValueCount = m_ClearValues.size();
   renderPassBeginInfo.pClearValues = m_ClearValues.data();
   renderPassBeginInfo.renderArea.offset = {0, 0};
   renderPassBeginInfo.renderArea.extent = framebuffer.Extent();
   renderPassBeginInfo.framebuffer = framebuffer.data();
   vkCmdBeginRenderPass(cmdBuffer.data(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}
