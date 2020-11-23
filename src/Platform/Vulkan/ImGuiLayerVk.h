#ifndef GAME_ENGINE_IMGUILAYERVK_H
#define GAME_ENGINE_IMGUILAYERVK_H

#include <Engine/ImGui/ImGuiLayer.h>
#include <Engine/Renderer/vulkan_wrappers.h>


class GfxContextVk;
class Device;


class ImGuiLayerVk : public ImGuiLayer {
private:
    GfxContextVk &m_Context;
    Device &m_Device;

    vk::DescriptorPool *m_DescriptorPool = nullptr;
    VkRenderPass m_RenderPass;

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
    ImGuiLayerVk();

    ~ImGuiLayerVk() override;

    void OnAttach(const LayerStack* stack) override;

    // Update vertex and index buffer containing the imGui elements when required
    // void UpdateBuffers(size_t frameIndex);

    // Draw current ImGui frame into a command buffer
    auto RecordBuffer(size_t index) -> VkCommandBuffer;

    void OnUpdate(Timestep) override {}

    void NewFrame() override;

    void EndFrame() override;
};


#endif //GAME_ENGINE_IMGUILAYERVK_H
