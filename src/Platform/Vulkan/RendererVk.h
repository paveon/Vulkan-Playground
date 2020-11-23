#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <memory>
#include <array>
#include <iostream>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <mathlib.h>
#include <vulkan/vulkan.h>
#include <Engine/Window.h>
#include <Engine/Renderer/Renderer.h>
#include <Engine/Renderer/RendererAPI.h>

#include "Engine/Renderer/vulkan_wrappers.h"
#include "Engine/Renderer/Device.h"
#include "Platform/Vulkan/GraphicsContextVk.h"
#include "Engine/Renderer/RenderPass.h"
#include "ImGuiLayerVk.h"


class WindowResizeEvent;

class RendererVk : public Renderer {
    struct MeshAllocationMetadata {
        vk::DeviceBuffer *buffer = nullptr;
        VkDeviceSize startOffset = 0;
    };


public:
    explicit RendererVk();

    ~RendererVk() override {
        vkDeviceWaitIdle(m_Device);
    }

    auto AcquireNextImage() -> uint32_t;

    void DrawFrame();

    void RecreateSwapchain();

    auto GetDevice() const -> Device & { return m_Device; }

    auto GetFramebuffer() -> vk::Framebuffer & { return *m_Framebuffers[m_ImageIndex]; }

    auto FramebufferSize() const -> std::pair<uint32_t, uint32_t> { return m_Context.FramebufferSize(); }

    void impl_OnWindowResize(WindowResizeEvent &e) override;

    auto impl_GetImageIndex() const -> size_t override { return m_ImageIndex; }

//    void impl_StageData(void* dstBuffer, uint64_t* dstOffset, const void *data, uint64_t bytes) override {
//        m_StageBuffer.StageData(static_cast<vk::Buffer **>(dstBuffer), dstOffset, data, bytes);
//    }

    void impl_StageMesh(Mesh *mesh) override {
        /// TODO: will break when mesh is released
        m_MeshAllocations[mesh->MeshID()] = MeshAllocationMetadata();
        m_StageBuffer.StageMesh(mesh);
    }

    auto impl_AllocateUniformBuffer(uint64_t size) -> BufferAllocation override {
        return {m_UniformBuffer.memory(),
                m_UniformBuffer.buffer(),
                m_UniformBuffer.SubAllocate(size)
        };
    }

    void impl_FlushStagedData() override;

    void impl_WaitIdle() const override;

    auto impl_GetRenderPass() const -> const RenderPass & override { return *m_RenderPass; }

private:
    const size_t MAX_FRAMES_IN_FLIGHT = 2;

    GfxContextVk &m_Context;
    Device &m_Device;

    /// TODO: Create some basic device memory management system and maybe a streaming system later?
    std::unordered_map<size_t, MeshAllocationMetadata> m_MeshAllocations;
    vk::RingStageBuffer m_StageBuffer;
    vk::DeviceBuffer m_MeshDeviceBuffer;
    vk::UniformBuffer m_UniformBuffer;

    vk::CommandPool *m_GfxCmdPool;
    vk::CommandPool *m_TransferCmdPool;
    vk::CommandBuffers *m_GfxCmdBuffers;
    vk::CommandBuffers *m_TransferCmdBuffers;

    vk::DeviceMemory *m_ImageMemory;
    vk::Image *m_ColorImage;
    vk::Image *m_DepthImage;
    vk::ImageView *m_ColorImageView;
    vk::ImageView *m_DepthImageView;

    std::vector<vk::Framebuffer *> m_Framebuffers;

    std::vector<vk::Semaphore *> m_AcquireSemaphores;
    std::vector<vk::Semaphore *> m_ReleaseSemaphores;
    std::vector<vk::Fence *> m_Fences;

    std::unique_ptr<RenderPass> m_RenderPass;

    std::array<VkClearValue, 2> m_ClearValues = {};
    VkViewport m_Viewport = {};
    VkRect2D m_Scissor = {};

    size_t m_FrameIndex = 0;
    uint32_t m_ImageIndex = 0;

    std::vector<VkBufferMemoryBarrier> m_TransferBarriers;

    void createSyncObjects();

    void impl_NextFrame() override {
        AcquireNextImage();
        if (m_ImGuiLayer) m_ImGuiLayer->NewFrame();
    }

    void impl_PresentFrame() override {
        if (m_ImGuiLayer) m_ImGuiLayer->EndFrame();
        DrawFrame();
    }
};


#endif //VULKAN_RENDERER_H
