#ifndef GAME_ENGINE_IMGUILAYERVK_H
#define GAME_ENGINE_IMGUILAYERVK_H

#include <Engine/ImGui/ImGuiLayer.h>
#include <Engine/Application.h>
#include "GraphicsContextVk.h"

//struct PushConstBlock {
//    math::vec2 scale;
//    math::vec2 translate;
//} pushConstBlock;


class ImGuiLayerVk : public ImGuiLayer {
private:
    GfxContextVk &m_Context;
    Device &m_Device;

    vk::DescriptorPool *m_DescriptorPool = nullptr;
    VkRenderPass m_RenderPass;
//    VkRenderPass m_RenderPass = nullptr;

    vk::CommandPool *m_CmdPool = nullptr;
    vk::CommandBuffers *m_CmdBuffers = nullptr;
    std::vector<vk::Framebuffer *> m_Framebuffers;

//    std::vector<vk::Buffer> m_VertexBuffers;
//    std::vector<vk::Buffer> m_IndexBuffers;
//    std::vector<vk::DeviceMemory> m_VertexMemories;
//    std::vector<vk::DeviceMemory> m_IndexMemories;
//    std::unique_ptr<Texture2D> m_FontData;
//    std::unique_ptr<Pipeline> m_Pipeline;


    // Initialize all Vulkan resources used by the ui
    void InitResources();

public:
    explicit ImGuiLayerVk() :
            ImGuiLayer(),
            m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
            m_Device(m_Context.GetDevice()),
            m_RenderPass(nullptr)
//            m_RenderPass((VkRenderPass)m_Renderer.GetRenderPass().VkHandle())
            {}

    ~ImGuiLayerVk() override;

    void OnAttach(const LayerStack* stack) override {
        ImGuiLayer::OnAttach(stack);
        ImGuiLayer::ConfigureImGui(m_Context.FramebufferSize());
        InitResources();
    }

    // Update vertex and index buffer containing the imGui elements when required
    // void UpdateBuffers(size_t frameIndex);

    // Draw current ImGui frame into a command buffer
    auto RecordBuffer(size_t index) -> VkCommandBuffer;

    void OnUpdate(Timestep) override {}

    void NewFrame() override;

    void EndFrame() override;
};


#endif //GAME_ENGINE_IMGUILAYERVK_H
