#define GLFW_INCLUDE_VULKAN

//#define MTH_DEPTH_NEGATIVE_TO_POSITIVE_ONE

#include <iostream>
#include <cstring>
#include <cassert>
#include <vector>
#include <set>

#include <GLFW/glfw3.h>
#include <mathlib.h>
#include <Platform/Vulkan/TextureVk.h>
#include <Platform/Vulkan/ShaderProgramVk.h>
#include <Platform/Vulkan/PipelineVk.h>
#include "Engine/ImGui/ImGuiLayer.h"

#include "utils.h"
#include "renderer.h"


GraphicsAPI Renderer::s_API = GraphicsAPI::VULKAN;


Renderer::Renderer(GfxContextVk &ctx) : m_Context(ctx), m_Device(ctx.GetDevice()) {
    m_RenderPass = RenderPass::Create();

    m_GfxCmdPool = m_Device.createCommandPool(m_Device.queueIndex(QueueFamily::GRAPHICS),
                                              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_TransferCmdPool = m_Device.createCommandPool(m_Device.queueIndex(QueueFamily::TRANSFER),
                                                   VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

    std::set<uint32_t> queueIndices{
            m_Device.queueIndex(QueueFamily::TRANSFER),
            m_Device.queueIndex(QueueFamily::GRAPHICS),
    };

    VkExtent2D extent = ctx.Swapchain().Extent();

    m_ColorImage = m_Device.createImage({m_Device.queueIndex(QueueFamily::GRAPHICS)}, extent, 1,
                                        Device::maxSamples(),
                                        ctx.Swapchain().ImageFormat(), VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    m_ColorImageMemory = m_Device.allocateImageMemory(*m_ColorImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkBindImageMemory(m_Device, m_ColorImage->data(), m_ColorImageMemory->data(), 0);
    m_ColorImageView = m_Device.createImageView(*m_ColorImage, VK_IMAGE_ASPECT_COLOR_BIT);

    /* Create depth test resources */
    VkFormat depthFormat = m_Device.findDepthFormat();
    m_DepthImage = m_Device.createImage({m_Device.queueIndex(QueueFamily::GRAPHICS)}, extent, 1,
                                        Device::maxSamples(),
                                        depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    m_DepthImageMemory = m_Device.allocateImageMemory(*m_DepthImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkBindImageMemory(m_Device, m_DepthImage->data(), m_DepthImageMemory->data(), 0);
    m_DepthImageView = m_Device.createImageView(*m_DepthImage, VK_IMAGE_ASPECT_DEPTH_BIT);


    //StagingBuffer textureBuffer(m_Device, texture->Data(), texture->Size());
    //    vk::CommandBuffer setupCmdBuffer(m_Device, m_TransferCmdPool.data());


//    vk::CommandBuffers setupCmdBuffers(m_Device, m_TransferCmdPool->data());
//    vk::CommandBuffer setupCmdBuffer = setupCmdBuffers[0];
//    setupCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    /* Transition texture image */
    //m_TextureImage.ChangeLayout(setupCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    //copyBufferToImage(setupCmdBuffer, textureBuffer, m_TextureImage);


    /* Transfer queue ownership of resources */
//    std::array<VkBufferMemoryBarrier, 2> bufferBarriers = {};
//    bufferBarriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
//    bufferBarriers[0].srcQueueFamilyIndex = m_Device.queueIndex(QueueFamily::TRANSFER);
//    bufferBarriers[0].dstQueueFamilyIndex = m_Device.queueIndex(QueueFamily::GRAPHICS);
//    bufferBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//    bufferBarriers[0].dstAccessMask = 0;
//    bufferBarriers[0].buffer = m_VertexBuffer.data().data();
//    bufferBarriers[0].size = m_VertexBuffer.size();
//    bufferBarriers[0].offset = 0;
//    bufferBarriers[1] = bufferBarriers[0];
//    bufferBarriers[1].buffer = m_IndexBuffer.data().data();
//    bufferBarriers[1].size = m_IndexBuffer.size();

//   std::array<VkImageMemoryBarrier, 1> imageBarriers = {};
//   imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//   //imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//   //imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//   imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//   imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//   imageBarriers[0].srcQueueFamilyIndex = m_Device.queueIndex(QueueFamily::TRANSFER);
//   imageBarriers[0].dstQueueFamilyIndex = m_Device.queueIndex(QueueFamily::GRAPHICS);
//   imageBarriers[0].image = m_TextureImage.data();
//   imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//   imageBarriers[0].subresourceRange.baseMipLevel = 0;
//   imageBarriers[0].subresourceRange.levelCount = m_TextureImage.MipLevels();
//   imageBarriers[0].subresourceRange.baseArrayLayer = 0;
//   imageBarriers[0].subresourceRange.layerCount = 1;
//   imageBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//   imageBarriers[0].dstAccessMask = 0;

//    vkCmdPipelineBarrier(setupCmdBuffer.data(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
//                         0, nullptr,
//                         bufferBarriers.size(), bufferBarriers.data(),
//                         0, nullptr
//    );
//
//    setupCmdBuffer.End();

//    vk::Semaphore transferSemaphore(m_Device);
//    VkSubmitInfo submitInfo = {};
//    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//    submitInfo.signalSemaphoreCount = 1;
//    submitInfo.pSignalSemaphores = transferSemaphore.ptr();
//    setupCmdBuffer.Submit(submitInfo, m_Device.queue(QueueFamily::TRANSFER));
//
    vk::CommandBuffers acquisitionCmdBuffers(m_Device, m_GfxCmdPool->data());
    vk::CommandBuffer acquisitionCmdBuffer = acquisitionCmdBuffers[0];
    acquisitionCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    m_ColorImage->ChangeLayout(acquisitionCmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    m_DepthImage->ChangeLayout(acquisitionCmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

//    bufferBarriers[0].srcAccessMask = 0;
//    bufferBarriers[0].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
//    bufferBarriers[1].srcAccessMask = 0;
//    bufferBarriers[1].dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
    //imageBarriers[0].srcAccessMask = 0;
    //imageBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//    vkCmdPipelineBarrier(acquisitionCmdBuffer.data(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
//                         VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
//                         0, nullptr,
//                         bufferBarriers.size(), bufferBarriers.data(),
//                         0, nullptr
//    );

//   vkCmdPipelineBarrier(acquisitionCmdBuffer.data(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
//                        0,
//                        0, nullptr,
//                        0, nullptr,
//                        imageBarriers.size(), imageBarriers.data()
//   );


    //m_TextureImage.GenerateMipmaps(m_Device, acquisitionCmdBuffer);

    acquisitionCmdBuffer.End();
    acquisitionCmdBuffer.Submit(m_Device.queue(QueueFamily::GRAPHICS));

    vkQueueWaitIdle(m_Device.queue(QueueFamily::GRAPHICS));

    std::vector<VkImageView> attachments{nullptr, m_DepthImageView->data()};
    if (Device::maxSamples() != VK_SAMPLE_COUNT_1_BIT) {
        attachments.push_back(m_ColorImageView->data());
    }

    for (const vk::ImageView &view : ctx.Swapchain().ImageViews()) {
        attachments.front() = view.data();
        auto *buffer = m_Device.createFramebuffer(extent, (VkRenderPass) m_RenderPass->VkHandle(), attachments);
        m_Framebuffers.push_back(buffer);
    }

//    createUniformBuffers(ctx.Swapchain().ImageCount());
//    m_GraphicsPipeline->BindUniformBuffer(m_UniformBuffers, 0);
//    m_GraphicsPipeline->BindTexture(*m_Texture, 1);

    //updateDescriptorSets(m_DescriptorSets);
    m_GraphicsCmdBuffers = m_Device.createCommandBuffers(*m_GfxCmdPool, m_Framebuffers.size());
    //recordCommandBuffers(m_GraphicsCmdBuffers);
    createSyncObjects();
}


Renderer::~Renderer() {
    vkDeviceWaitIdle(m_Device);
    std::cout << "[Renderer] Stopped rendering" << std::endl;
}

void Renderer::recordCommandBuffer(VkCommandBuffer cmdBuffer, size_t index) {
    auto extent = m_Context.Swapchain().Extent();

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkClearColorValue colorClear = {{0.0f, 0.0f, 0.0f, 1.0f}};
    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = colorClear;
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = (VkRenderPass)m_RenderPass->VkHandle();
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = extent;
    renderPassBeginInfo.clearValueCount = clearValues.size();
    renderPassBeginInfo.pClearValues = clearValues.data();
    renderPassBeginInfo.framebuffer = m_Framebuffers[index]->data();

    if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

//   VkImageMemoryBarrier renderBeginBarrier = imageBarrier(m_Swapchain.Images()[index], 0,
//                                                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
//                                                          VK_IMAGE_LAYOUT_UNDEFINED,
//                                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
//
//   vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
//                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//                        VK_DEPENDENCY_BY_REGION_BIT,
//                        0, nullptr, 0, nullptr, 1, &renderBeginBarrier);

    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

//    m_GraphicsPipeline->Bind(cmdBuffer, index);
//
//    VkDeviceSize bufferOffset = 0;
//    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, m_VertexBuffer.data().ptr(), &bufferOffset);
//
//    vkCmdBindIndexBuffer(cmdBuffer, m_IndexBuffer.data().data(), 0, VK_INDEX_TYPE_UINT32);
//
//    vkCmdDrawIndexed(cmdBuffer, m_Model->IndexCount(), 1, 0, 0, 0);

    vkCmdEndRenderPass(cmdBuffer);

//      VkImageMemoryBarrier renderEndBarrier = imageBarrier(m_SwapchainImages[i],
//                                                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
//                                                           0,
//                                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
//                                                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
//
//      vkCmdPipelineBarrier(cmdBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//                           VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT,
//                           0, nullptr,
//                           0, nullptr,
//                           1, &renderEndBarrier);

    if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}


void Renderer::createSyncObjects() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_AcquireSemaphores.emplace_back(m_Device.createSemaphore());
        m_ReleaseSemaphores.emplace_back(m_Device.createSemaphore());
        m_Fences.emplace_back(m_Device.createFence(true));
    }
}


void Renderer::createUniformBuffers(size_t count) {
//    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
//
//    VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
//
//    m_UniformBuffers.reserve(count);
//    m_UniformMemory.reserve(count);
//    for (size_t i = 0; i < count; i++) {
//        m_UniformBuffers.emplace_back(m_Device.createBuffer({m_Device.queueIndex(QueueFamily::GRAPHICS)}, bufferSize,
//                                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
//
//        m_UniformMemory.emplace_back(m_Device.allocateBufferMemory(*m_UniformBuffers.back(), memoryFlags));
//        vkBindBufferMemory(m_Device, m_UniformBuffers.back()->data(), m_UniformMemory.back()->data(), 0);
//    }
}


//void Renderer::cleanupSwapchain() {
//    //vkDestroyPipeline(Device, m_GraphicsPipeline, nullptr);
//    //vkDestroyPipelineLayout(Device, m_PipelineLayout, nullptr);
//
//    m_Framebuffers.clear();
//    m_Swapchain.Release();
//
//    if (m_RenderPass) m_RenderPass.reset();
//}


void Renderer::RecreateSwapchain() {
    vkDeviceWaitIdle(m_Device);

    if (m_RenderPass) m_RenderPass.reset();
    m_Context.RecreateSwapchain();
    auto extent = m_Context.Swapchain().Extent();

    m_RenderPass = RenderPass::Create();

    VkFormat depthFormat = m_Device.findDepthFormat();
    m_DepthImage = m_Device.createImage({m_Device.queueIndex(QueueFamily::GRAPHICS)}, extent, 1,
                                        m_Device.maxSamples(), depthFormat,
                                        VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    m_DepthImageMemory = m_Device.allocateImageMemory(*m_DepthImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkBindImageMemory(m_Device, m_DepthImage->data(), m_DepthImageMemory->data(), 0);
    m_DepthImageView = m_Device.createImageView(*m_DepthImage, VK_IMAGE_ASPECT_DEPTH_BIT);

    m_ColorImage = m_Device.createImage({m_Device.queueIndex(QueueFamily::GRAPHICS)}, extent, 1,
                                        m_Device.maxSamples(),
                                        m_Context.Swapchain().ImageFormat(), VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    m_ColorImageMemory = m_Device.allocateImageMemory(*m_ColorImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkBindImageMemory(m_Device, m_ColorImage->data(), m_ColorImageMemory->data(), 0);
    m_ColorImageView = m_Device.createImageView(*m_ColorImage, VK_IMAGE_ASPECT_COLOR_BIT);

    vk::CommandBuffers acquisitionBuffers(m_Device, m_GfxCmdPool->data());
    vk::CommandBuffer acquisitionBuffer = acquisitionBuffers[0];
    acquisitionBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    m_ColorImage->ChangeLayout(acquisitionBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    m_DepthImage->ChangeLayout(acquisitionBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    acquisitionBuffer.End();
    acquisitionBuffer.Submit(m_Device.queue(QueueFamily::GRAPHICS));

    vkQueueWaitIdle(m_Device.queue(QueueFamily::GRAPHICS));

    std::vector<VkImageView> attachments{nullptr, m_DepthImageView->data()};
    if (Device::maxSamples() != VK_SAMPLE_COUNT_1_BIT) {
        attachments.push_back(m_ColorImageView->data());
    }

    m_Framebuffers.clear();
    for (const vk::ImageView &view : m_Context.Swapchain().ImageViews()) {
        attachments.front() = view.data();
        auto *buffer = m_Device.createFramebuffer(extent, (VkRenderPass)m_RenderPass->VkHandle(), attachments);
        m_Framebuffers.push_back(buffer);
    }

    m_GraphicsCmdBuffers = m_Device.createCommandBuffers(*m_GfxCmdPool, m_Framebuffers.size());
}


//void Renderer::updateUniformBuffer(uint32_t currentImage) {
//    static auto startTime = TIME_NOW;
//
//    auto currentTime = TIME_NOW;
//    float time = std::chrono::duration<float, std::chrono::seconds::period>(
//            currentTime - startTime).count();
//
//    UniformBufferObject ubo = {};
//    ubo.model = math::rotate(ubo.model, time * math::radians(15.0f), math::vec3(0.0f, 0.0f, 1.0f));
//
//    math::vec3 cameraPos(2.0f, 2.0f, 2.0f);
//    math::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
//    math::vec3 upVector(0.0f, 0.0f, 1.0f);
//    ubo.view = math::lookAt(cameraPos, cameraTarget, upVector);
//
//    auto &ctx = static_cast<GfxContextVk &>(m_Context);
//    auto extent = ctx.Swapchain().Extent();
//
//    float verticalFov = math::radians(45.0f);
//    float aspectRatio = extent.width / (float) extent.height;
//    float nearPlane = 0.1f;
//    float farPlane = 10.0f;
//    ubo.proj = math::perspective(verticalFov, aspectRatio, nearPlane, farPlane);
//    ubo.proj[1][1] *= -1;
//
//    void *data = nullptr;
//    vkMapMemory(m_Device, m_UniformMemory[currentImage]->data(), 0, sizeof(ubo), 0, &data);
//    memcpy(data, &ubo, sizeof(ubo));
//    vkUnmapMemory(m_Device, m_UniformMemory[currentImage]->data());
//}

auto Renderer::AcquireNextImage() -> uint32_t {
    auto &ctx = static_cast<GfxContextVk &>(m_Context);
    VkResult result = vkAcquireNextImageKHR(m_Device, ctx.Swapchain().data(), UINT64_MAX,
                                            m_AcquireSemaphores[m_FrameIndex]->data(), nullptr,
                                            &m_ImageIndex);

    vkWaitForFences(m_Device, 1, m_Fences[m_FrameIndex]->ptr(), VK_TRUE, UINT64_MAX);
    vkResetFences(m_Device, 1, m_Fences[m_FrameIndex]->ptr());

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
//        std::cout << "OUT_OF_DATE" << std::endl;
//        RecreateSwapchain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire next swapchain image!");
    }

    return m_ImageIndex;
}

void Renderer::DrawFrame(std::vector<VkCommandBuffer> &submitBuffers) {
//    submitBuffers.insert(submitBuffers.begin(), m_GraphicsCmdBuffers->get(m_ImageIndex).data());
//    recordCommandBuffer(submitBuffers.front(), m_ImageIndex);


    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = m_AcquireSemaphores[m_FrameIndex]->ptr();
    submitInfo.pWaitDstStageMask = &waitStages;
    submitInfo.commandBufferCount = submitBuffers.size();
    submitInfo.pCommandBuffers = submitBuffers.data();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = m_ReleaseSemaphores[m_FrameIndex]->ptr();

//    vkResetFences(m_Device, 1, m_Fences[m_FrameIndex].ptr());

    if (vkQueueSubmit(m_Device.GfxQueue(), 1, &submitInfo, m_Fences[m_FrameIndex]->data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    auto &ctx = static_cast<GfxContextVk &>(m_Context);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = m_ReleaseSemaphores[m_FrameIndex]->ptr();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = ctx.Swapchain().ptr();
    presentInfo.pImageIndices = &m_ImageIndex;

    VkResult result = vkQueuePresentKHR(m_Device.GfxQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
//        std::cout << "OUT_OF_DATE || SUBOPTIMAL" << std::endl;
//        RecreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_FrameIndex = (m_FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::OnWindowResize(WindowResizeEvent&) {
    RecreateSwapchain();
}
