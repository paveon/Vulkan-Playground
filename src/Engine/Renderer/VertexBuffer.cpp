#include "VertexBuffer.h"
#include <Platform/Vulkan/VertexBufferVk.h>
#include "Renderer.h"


auto VertexBuffer::Create() -> std::unique_ptr<VertexBuffer> {
    switch (Renderer::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<VertexBufferVk>();
    }

    return nullptr;
}
