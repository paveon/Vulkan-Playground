#include "GraphicsContext.h"
#include "Platform/Vulkan/GraphicsContextVk.h"

#include <iostream>
#include <Engine/Renderer/RendererAPI.h>

auto GfxContext::Create(void* windowHandle) -> std::unique_ptr<GfxContext> {
   switch (RendererAPI::GetSelectedAPI()) {
       case RendererAPI::API::VULKAN: return std::make_unique<GfxContextVk>(static_cast<GLFWwindow*>(windowHandle));
   }

   return nullptr;
}

GfxContext::~GfxContext() {
    std::cout << "[GfxContext] Destroying rendering context" << std::endl;
}
