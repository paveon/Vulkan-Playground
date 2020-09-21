#include "UniformBuffer.h"
#include <Platform/Vulkan/UniformBufferVk.h>
#include "RendererAPI.h"

auto UniformBuffer::Create(size_t objectByteSize) -> std::unique_ptr<UniformBuffer> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<UniformBufferVk>(objectByteSize);
    }

    return nullptr;
}