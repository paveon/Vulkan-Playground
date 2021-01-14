#ifndef VULKAN_TEXTUREVK_H
#define VULKAN_TEXTUREVK_H

#include "Engine/Renderer/vulkan_wrappers.h"
#include "Engine/Renderer/Texture.h"
#include "ShaderPipelineVk.h"

class Texture2DVk : public Texture2D {
private:
    static std::unique_ptr<RenderPassVk> s_Renderpass;
    static std::unique_ptr<ShaderPipelineVk> s_BrdfLutPipeline;

    vk::Image *m_TextureImage{};
    vk::DeviceMemory *m_TextureMemory{};
    vk::ImageView *m_TextureView{};

    static void InitResources();

public:
    Texture2DVk(const u_char *data, uint32_t width, uint32_t height, uint32_t channels, VkFormat format);

    Texture2DVk(uint32_t resolution, uint32_t channels, VkFormat format);

    void Upload() override;

    auto View() const -> const vk::ImageView & { return *m_TextureView; }

    static std::unique_ptr<Texture2DVk> GenerateBrdfLut(uint32_t resolution);
};


class TextureCubemapVk : public TextureCubemap {
private:
    static std::unique_ptr<RenderPassVk> s_CubemapRenderpass;
    static std::unique_ptr<ShaderPipelineVk> s_CubemapPipeline;
    static std::unique_ptr<ShaderPipelineVk> s_IrradiancePipeline;
    static std::unique_ptr<ShaderPipelineVk> s_PrefilterPipeline;
    static std::unique_ptr<vk::Buffer> s_CubeVertexBuffer;
    static std::unique_ptr<vk::DeviceMemory> s_CubeVertexMemory;
    static std::array<glm::mat4, 6> s_CaptureViews;

    vk::Image *m_TextureImage{};
    vk::DeviceMemory *m_TextureMemory{};
    vk::ImageView *m_TextureView{};

//    auto PrepareTextureImage(uint32_t width,
//                             uint32_t height,
//                             uint32_t maxMipLevels,
//                             VkFormat format,
//                             VkImageUsageFlags usage) -> vk::Image *;

    // TODO: bad approach, will get rid of it later on
    static void InitResources();

public:
    TextureCubemapVk(std::array<u_char *, 6> data, uint32_t width, uint32_t height, uint32_t channels);

    TextureCubemapVk(const float *data, uint32_t width, uint32_t height, uint32_t channels,
                     uint32_t faceResolution);

    TextureCubemapVk(const u_char *data, uint32_t width, uint32_t height, uint32_t channels);

    TextureCubemapVk(uint32_t width, uint32_t height, uint32_t maxMipLevels);

    void Upload() override;

    void HDRtoCubemap() override;

    auto CreateIrradianceCubemap(uint32_t resolution) -> TextureCubemap * override;

    auto CreatePrefilteredCubemap(uint32_t baseResolution, uint32_t maxMipLevels) -> TextureCubemap * override;

    auto View() const -> const vk::ImageView & { return *m_TextureView; }
};


#endif //VULKAN_TEXTUREVK_H
