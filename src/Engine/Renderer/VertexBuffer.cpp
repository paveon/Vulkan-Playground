#include "VertexBuffer.h"

#include <Platform/Vulkan/VertexBufferVk.h>
#include "renderer.h"

auto VertexBuffer::Create() -> std::unique_ptr<VertexBuffer> {
    switch (Renderer::GetCurrentAPI()) {
        case GraphicsAPI::VULKAN:
            return std::make_unique<VertexBufferVk>();
    }

    return nullptr;
}