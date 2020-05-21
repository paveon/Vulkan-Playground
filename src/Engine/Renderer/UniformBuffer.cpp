#include "UniformBuffer.h"

#include <Platform/Vulkan/UniformBufferVk.h>
#include "renderer.h"

auto UniformBuffer::Create(size_t objectByteSize) -> std::unique_ptr<UniformBuffer> {
    switch (Renderer::GetCurrentAPI()) {
        case GraphicsAPI::VULKAN:
            return std::make_unique<UniformBufferVk>(objectByteSize);
    }

    return nullptr;
}