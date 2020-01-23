#include "RenderPass.h"

#include <Platform/Vulkan/RenderPassVk.h>
#include "renderer.h"


std::unique_ptr<RenderPass> RenderPass::Create() {
   switch (Renderer::GetCurrentAPI()) {
      case GraphicsAPI::VULKAN:
         return std::make_unique<RenderPassVk>();
   }

   return nullptr;
}