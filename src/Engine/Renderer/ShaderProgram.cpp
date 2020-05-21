#include "ShaderProgram.h"

#include <Platform/Vulkan/ShaderProgramVk.h>
#include "renderer.h"


auto ShaderProgram::Create(const char* filepath) -> std::unique_ptr<ShaderProgram> {
   switch (Renderer::GetCurrentAPI()) {
      case GraphicsAPI::VULKAN:
         return std::make_unique<ShaderProgramVk>(filepath);
   }

   return nullptr;
}