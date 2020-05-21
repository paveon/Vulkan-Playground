#define GLFW_INCLUDE_VULKAN

//#define MTH_DEPTH_NEGATIVE_TO_POSITIVE_ONE

#include <iostream>
#include <cstring>
#include <cassert>
#include <vector>
#include <set>

//#include <GLFW/glfw3.h>
//#include "Engine/ImGui/ImGuiLayer.h"

#include "renderer.h"


GraphicsAPI Renderer::s_API = GraphicsAPI::VULKAN;


Renderer::Renderer(GfxContextVk &ctx) : m_Context(ctx), m_Device(ctx.GetDevice()) {
    vk::Swapchain &swapchain(ctx.Swapchain());
    auto extent = swapchain.Extent();
    auto imgFormat = swapchain.ImageFormat();
    auto maxImgCount = swapchain.Capabilities().maxImageCount;

    m_RenderPass = RenderPass::Create();

    m_GfxCmdPool = m_Device.createCommandPool(m_Device.queueIndex(QueueFamily::GRAPHICS),
                                              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_TransferCmdPool = m_Device.createCommandPool(m_Device.queueIndex(QueueFamily::TRANSFER),
                                                   VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

    m_GfxCmdBuffers = m_Device.createCommandBuffers(*m_GfxCmdPool, maxImgCount);
    m_TransferCmdBuffers = m_Device.createCommandBuffers(*m_TransferCmdPool, maxImgCount);

    std::set<uint32_t> queueIndices{
            m_Device.queueIndex(QueueFamily::TRANSFER),
            m_Device.queueIndex(QueueFamily::GRAPHICS),
    };

    VkExtent2D maxExtent{1920, 1080 }; // Temporary hack

    m_ColorImage = m_Device.createImage(
            {m_Device.queueIndex(QueueFamily::GRAPHICS)},
            maxExtent, 1,
            Device::maxSamples(),
            imgFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    m_DepthImage = m_Device.createImage(
            {m_Device.queueIndex(QueueFamily::GRAPHICS)},
            maxExtent, 1,
            Device::maxSamples(),
            m_Device.findDepthFormat(), VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    m_ImageMemory = m_Device.allocateImageMemory({m_ColorImage, m_DepthImage}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    auto alignment = roundUp(m_ColorImage->MemoryInfo().size, m_DepthImage->MemoryInfo().alignment);
    m_ColorImage->BindMemory(m_ImageMemory->data(), 0);
    m_DepthImage->BindMemory(m_ImageMemory->data(), alignment);

    m_ColorImageView = m_Device.createImageView(*m_ColorImage, VK_IMAGE_ASPECT_COLOR_BIT);
    m_DepthImageView = m_Device.createImageView(*m_DepthImage, VK_IMAGE_ASPECT_DEPTH_BIT);

    vk::CommandBuffer setupCmdBuffer = m_GfxCmdBuffers->get(0);
    setupCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    m_ColorImage->ChangeLayout(setupCmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    m_DepthImage->ChangeLayout(setupCmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    setupCmdBuffer.End();
    setupCmdBuffer.Submit(m_Device.queue(QueueFamily::GRAPHICS));

    vkQueueWaitIdle(m_Device.queue(QueueFamily::GRAPHICS));

    std::vector<VkImageView> attachments{nullptr, m_DepthImageView->data()};
    if (Device::maxSamples() != VK_SAMPLE_COUNT_1_BIT) {
        attachments.push_back(m_ColorImageView->data());
    }

    for (const vk::ImageView &view : swapchain.ImageViews()) {
        attachments.front() = view.data();
        auto *buffer = m_Device.createFramebuffer(extent, (VkRenderPass) m_RenderPass->VkHandle(), attachments);
        m_Framebuffers.push_back(buffer);
    }

    createSyncObjects();
}


Renderer::~Renderer() {
    vkDeviceWaitIdle(m_Device);
    std::cout << "[Renderer] Stopped rendering" << std::endl;
}


void Renderer::createSyncObjects() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_AcquireSemaphores.emplace_back(m_Device.createSemaphore());
        m_ReleaseSemaphores.emplace_back(m_Device.createSemaphore());
        m_Fences.emplace_back(m_Device.createFence(true));
    }
}


void Renderer::RecreateSwapchain() {
    vkDeviceWaitIdle(m_Device);

    m_Context.RecreateSwapchain();
    auto extent = m_Context.Swapchain().Extent();

    m_RenderPass = RenderPass::Create();

    // TODO: Image or depth format might possibly change with swapchain recreation
    std::vector<VkImageView> attachments{nullptr, m_DepthImageView->data()};
    if (Device::maxSamples() != VK_SAMPLE_COUNT_1_BIT) {
        attachments.push_back(m_ColorImageView->data());
    }

    m_Framebuffers.clear();
    for (const vk::ImageView &view : m_Context.Swapchain().ImageViews()) {
        attachments.front() = view.data();
        auto *buffer = m_Device.createFramebuffer(extent, (VkRenderPass) m_RenderPass->VkHandle(), attachments);
        m_Framebuffers.push_back(buffer);
    }
}


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

void Renderer::OnWindowResize(WindowResizeEvent &) {
    RecreateSwapchain();
}

