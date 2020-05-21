#include "ShaderProgramVk.h"

#include <Engine/Application.h>
#include "GraphicsContextVk.h"


ShaderProgramVk::ShaderProgramVk(const char* filepath) {
   auto& device = static_cast<GfxContextVk&>(Application::GetGraphicsContext()).GetDevice();
   m_ShaderModule = device.createShaderModule(filepath);
}
