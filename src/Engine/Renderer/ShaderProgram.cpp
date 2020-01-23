#include "ShaderProgram.h"

#include <Platform/Vulkan/ShaderProgramVk.h>
#include "renderer.h"


std::unique_ptr<ShaderProgram> ShaderProgram::Create(const char* filepath) {
   switch (Renderer::GetCurrentAPI()) {
      case GraphicsAPI::VULKAN:
         return std::make_unique<ShaderProgramVk>(filepath);
   }

   return nullptr;
}