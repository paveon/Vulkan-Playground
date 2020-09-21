#include "ShaderProgram.h"
#include <Platform/Vulkan/ShaderProgramVk.h>

#include "RendererAPI.h"


auto ShaderProgram::Create(const char* filepath) -> std::unique_ptr<ShaderProgram> {
   switch (RendererAPI::GetSelectedAPI()) {
       case RendererAPI::API::VULKAN:
         return std::make_unique<ShaderProgramVk>(filepath);
   }

   return nullptr;
}