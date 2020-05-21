#include <iostream>
#include "GraphicsContext.h"
#include "Platform/Vulkan/GraphicsContextVk.h"

#include "renderer.h"

std::unique_ptr<GfxContext> GfxContext::Create(void* windowHandle) {
   switch (Renderer::GetCurrentAPI()) {
      case GraphicsAPI::VULKAN: return std::make_unique<GfxContextVk>(static_cast<GLFWwindow*>(windowHandle));
   }

   return nullptr;
}

GfxContext::~GfxContext() {
    std::cout << "[GfxContext] Destroying rendering context" << std::endl;
}
