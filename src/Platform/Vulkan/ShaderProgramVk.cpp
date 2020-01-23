#include "ShaderProgramVk.h"

#include <Engine/Application.h>
#include "GraphicsContextVk.h"


ShaderProgramVk::ShaderProgramVk(const char* filepath) {
   auto& gfxContext = static_cast<const GfxContextVk&>(Application::GetGraphicsContext());
   m_ShaderModule = vk::ShaderModule(gfxContext.GetDevice(), filepath);
}
