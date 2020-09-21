#ifndef GAME_ENGINE_RENDERER_API_H
#define GAME_ENGINE_RENDERER_API_H

#include <mathlib.h>
#include "VertexBuffer.h"
#include "IndexBuffer.h"


class RendererAPI {
public:
    enum class API {
        VULKAN = 0
    };

private:
    static API s_SelectedAPI;

public:
    virtual ~RendererAPI() = default;

    static void SelectAPI(API api);

    static auto GetSelectedAPI() -> API { return s_SelectedAPI; }

    virtual void SetClearColor(const math::vec4 &color) = 0;

    virtual void Clear() = 0;

    virtual void DrawIndexed(const VertexBuffer& data, const IndexBuffer& indices) = 0;
};


#endif //GAME_ENGINE_RENDERER_API_H
