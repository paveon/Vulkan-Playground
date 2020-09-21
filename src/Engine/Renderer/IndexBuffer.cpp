#include "IndexBuffer.h"
#include <Platform/Vulkan/IndexBufferVk.h>

#include "RendererAPI.h"


auto IndexBuffer::Create() -> std::unique_ptr<IndexBuffer> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<IndexBufferVk>();
    }

    return nullptr;
}