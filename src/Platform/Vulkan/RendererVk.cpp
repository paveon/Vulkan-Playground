#define GLFW_INCLUDE_VULKAN

//#define MTH_DEPTH_NEGATIVE_TO_POSITIVE_ONE

#include <iostream>
#include <cstring>
#include <cassert>
#include <vector>
#include <set>
#include <Engine/Application.h>

#include "RendererVk.h"
#include <examples/imgui_impl_vulkan.h>
#include <Engine/Core.h>


RendererVk::RendererVk() :
        m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
        m_Device(m_Context.GetDevice()),
        m_StageBuffer(&m_Device, 1000'000'00),
        m_DataBuffer(&m_Device, 1000'000'00,
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
    vk::Swapchain &swapchain(m_Context.Swapchain());
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

    VkExtent2D maxExtent{1920, 1080}; // Temporary hack

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

    m_Viewport.width = static_cast<float>(extent.width);
    m_Viewport.height = static_cast<float>(extent.height);
    m_Viewport.maxDepth = 1.0f;
    m_Scissor.extent = extent;

    m_ClearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    m_ClearValues[1].depthStencil = {1.0f, 0};
}


void RendererVk::createSyncObjects() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_AcquireSemaphores.emplace_back(m_Device.createSemaphore());
        m_ReleaseSemaphores.emplace_back(m_Device.createSemaphore());
        m_Fences.emplace_back(m_Device.createFence(true));
    }
}


void RendererVk::RecreateSwapchain() {
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


auto RendererVk::AcquireNextImage() -> uint32_t {
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

void RendererVk::DrawFrame() {
    vk::CommandBuffer primaryCmdBuffer(m_GfxCmdBuffers->get(m_ImageIndex));

    primaryCmdBuffer.Begin();

    auto *currentFramebuffer = m_Framebuffers[m_ImageIndex]->data();
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = (VkRenderPass) m_RenderPass->VkHandle();
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.clearValueCount = m_ClearValues.size();
    renderPassBeginInfo.pClearValues = m_ClearValues.data();
    renderPassBeginInfo.renderArea.extent = m_Context.Swapchain().Extent();
    renderPassBeginInfo.framebuffer = currentFramebuffer;

    // The primary command buffer does not contain any rendering commands
    // These are stored (and retrieved) from the secondary command buffers
//    vkCmdBeginRenderPass(primaryCmdBuffer.data(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    vkCmdBeginRenderPass(primaryCmdBuffer.data(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(primaryCmdBuffer.data(), 0, 1, &m_Viewport);
    vkCmdSetScissor(primaryCmdBuffer.data(), 0, 1, &m_Scissor);

    RenderCommand *cmd = nullptr;
    while ((cmd = m_CmdQueue.GetNextCommand()) != nullptr) {
        switch (cmd->m_Type) {
            case RenderCommand::Type::SET_CLEAR_COLOR: {
                auto clearColor = cmd->UnpackData<math::vec4>();
                std::memcpy(m_ClearValues[0].color.float32, &clearColor, sizeof(math::vec4));
                break;
            }
            case RenderCommand::Type::SET_DEPTH_STENCIL: {
                auto depthStencil = cmd->UnpackData<DepthStencil>();
                std::memcpy(&m_ClearValues[1].depthStencil, &depthStencil, sizeof(DepthStencil));
                break;
            }
            case RenderCommand::Type::SET_VIEWPORT: {
                auto viewport = cmd->UnpackData<Viewport>();
                std::memcpy(&m_Viewport, &viewport, sizeof(Viewport));
                break;
            }
            case RenderCommand::Type::SET_SCISSOR: {
                auto scissor = cmd->UnpackData<Scissor>();
                std::memcpy(&m_Scissor, &scissor, sizeof(Scissor));
                break;
            }
            case RenderCommand::Type::BIND_VERTEX_BUFFER: {
                auto payload = cmd->UnpackData<BindVertexBufferPayload>();
                payload.vertexBuffer->Bind(primaryCmdBuffer.data(), payload.offset);
                break;
            }
            case RenderCommand::Type::BIND_INDEX_BUFFER: {
                auto payload = cmd->UnpackData<BindIndexBufferPayload>();
                payload.indexBuffer->Bind(primaryCmdBuffer.data(), payload.offset);
                break;
            }
            case RenderCommand::Type::BIND_MATERIAL: {
                auto *material = cmd->UnpackData<Material *>();
                material->GetPipeline().Bind(primaryCmdBuffer.data(), m_ImageIndex);
                break;
            }
            case RenderCommand::Type::DRAW_INDEXED: {
                auto payload = cmd->UnpackData<DrawIndexedPayload>();
                vkCmdDrawIndexed(primaryCmdBuffer.data(),
                                 payload.indexCount,
                                 1,
                                 payload.firstIndex,
                                 payload.vertexOffset,
                                 0);
                break;
            }

            case RenderCommand::Type::NONE:
            case RenderCommand::Type::CLEAR:
                break;
        }
    }

//    vkCmdExecuteCommands(primaryCmdBuffer.data(), submitBuffers.size(), submitBuffers.data());

    if (m_ImGuiLayer) {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), primaryCmdBuffer.data());
    }

    vkCmdEndRenderPass(primaryCmdBuffer.data());

    primaryCmdBuffer.End();

    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = m_AcquireSemaphores[m_FrameIndex]->ptr();
    submitInfo.pWaitDstStageMask = &waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = primaryCmdBuffer.ptr();
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

void RendererVk::impl_OnWindowResize(WindowResizeEvent &) {
    RecreateSwapchain();
}

void RendererVk::impl_WaitIdle() const {
    vkDeviceWaitIdle(m_Device);
    std::cout << currentTime() << "[Renderer] Idle state" << std::endl;
}

void RendererVk::impl_FlushStagedData() {
//    vkCmdCopyBuffer(cmdBuffer, srcBuffer.data(), dstBuffer.data(), copyRegions.size(), copyRegions.data());


//    std::array<VkBufferMemoryBarrier, 1> bufferBarriers = {};
//    vk::Semaphore transferSemaphore(m_Device);
//    VkSubmitInfo submitInfo = {};
//    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//
//    vk::CommandBuffers transferCmdBuffers(m_Device, m_Device.TransferPool()->data());
//    {
//
//        vk::CommandBuffer transferCmdBuffer = transferCmdBuffers[0];
//        transferCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
//
//        auto usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
//        m_Buffer = DeviceBuffer(&m_Device, data, bytes, transferCmdBuffer, usage);
//
//        bufferBarriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
//        bufferBarriers[0].srcQueueFamilyIndex = m_Device.queueIndex(QueueFamily::TRANSFER);
//        bufferBarriers[0].dstQueueFamilyIndex = m_Device.queueIndex(QueueFamily::GRAPHICS);
////        bufferBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//        bufferBarriers[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
//        bufferBarriers[0].dstAccessMask = 0;
//        bufferBarriers[0].buffer = m_Buffer.data().data();
//        bufferBarriers[0].size = m_Buffer.size();
//        bufferBarriers[0].offset = 0;
//
//        vkCmdPipelineBarrier(transferCmdBuffer.data(),
//                             VK_PIPELINE_STAGE_TRANSFER_BIT,
//                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
//                             0, nullptr,
//                             bufferBarriers.size(), bufferBarriers.data(),
//                             0, nullptr
//        );
//
//        transferCmdBuffer.End();
//
//        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//        submitInfo.signalSemaphoreCount = 1;
//        submitInfo.pSignalSemaphores = transferSemaphore.ptr();
//        transferCmdBuffer.Submit(submitInfo, m_Device.queue(QueueFamily::TRANSFER));
//    }
//
//    /* Acquisition phase */
//    {
//        vk::CommandBuffers cmds(m_Device, m_Device.GfxPool()->data());
//        vk::CommandBuffer gfxCmdBuffer = cmds[0];
//        gfxCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
//
//        bufferBarriers[0].srcAccessMask = 0;
//        bufferBarriers[0].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
//        vkCmdPipelineBarrier(gfxCmdBuffer.data(),
//                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
//                             VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
//                             0, nullptr,
//                             bufferBarriers.size(), bufferBarriers.data(),
//                             0, nullptr
//        );
//
//        gfxCmdBuffer.End();
//
//        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//        submitInfo.waitSemaphoreCount = 1;
//        submitInfo.pWaitSemaphores = transferSemaphore.ptr();
//        submitInfo.pWaitDstStageMask = &waitStage;
//        gfxCmdBuffer.Submit(submitInfo, m_Device.queue(QueueFamily::GRAPHICS));
//
//        vkQueueWaitIdle(m_Device.queue(QueueFamily::GRAPHICS));
//    }
}

