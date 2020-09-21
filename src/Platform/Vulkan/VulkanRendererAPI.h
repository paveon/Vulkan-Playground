#ifndef GAME_ENGINE_VULKAN_RENDERER_API_H
#define GAME_ENGINE_VULKAN_RENDERER_API_H

#include <Engine/Renderer/RendererAPI.h>


class VulkanRendererAPI : public RendererAPI {
private:


public:
    void SetClearColor(const math::vec4 &) override {};

    void Clear() override {};

    void DrawIndexed(const VertexBuffer&, const IndexBuffer&) override {};
};


#endif //GAME_ENGINE_VULKAN_RENDERER_API_H
