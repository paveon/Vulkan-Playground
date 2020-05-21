#include "IndexBuffer.h"

#include <Platform/Vulkan/IndexBufferVk.h>
#include "renderer.h"

auto IndexBuffer::Create() -> std::unique_ptr<IndexBuffer> {
    switch (Renderer::GetCurrentAPI()) {
        case GraphicsAPI::VULKAN:
            return std::make_unique<IndexBufferVk>();
    }

    return nullptr;
}