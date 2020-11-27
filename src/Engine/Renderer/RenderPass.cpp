#include "RenderPass.h"
#include <Platform/Vulkan/RenderPassVk.h>

#include "RendererAPI.h"


auto RenderPass::Create() -> std::unique_ptr<RenderPass> {
   switch (RendererAPI::GetSelectedAPI()) {
      case RendererAPI::API::VULKAN:
         return std::make_unique<RenderPassVk>(true);
   }

   return nullptr;
}