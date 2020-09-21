#include "RendererAPI.h"

#include <Platform/Vulkan/VulkanRendererAPI.h>
#include "RenderCommand.h"

RendererAPI::API RendererAPI::s_SelectedAPI =  RendererAPI::API::VULKAN;

void RendererAPI::SelectAPI(RendererAPI::API api) {
    switch (api) {
        case API::VULKAN:
            RenderCommand::s_API = std::make_unique<VulkanRendererAPI>();
            break;
    }
}
