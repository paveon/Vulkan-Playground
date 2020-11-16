#include "UniformBuffer.h"
#include <Platform/Vulkan/UniformBufferVk.h>
#include "RendererAPI.h"

auto UniformBuffer::Create(size_t objectSize, size_t objectCount) -> std::unique_ptr<UniformBuffer> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN: {
            auto ub = std::make_unique<UniformBufferVk>(objectSize, objectCount > 1);
            ub->Allocate(objectCount);
            return ub;
        }
    }

    return nullptr;
}