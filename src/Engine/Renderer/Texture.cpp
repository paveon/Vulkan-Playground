#include "Texture.h"
#include "renderer.h"
#include <stb_image.h>
#include <Platform/Vulkan/TextureVk.h>

Texture2D::Texture2D(const char* filepath) {
   m_Data = stbi_load(filepath, &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha);
   m_Size = m_Width * m_Height * 4;

   if (!m_Data) { throw std::runtime_error("failed to load texture image!"); }
}

Texture2D::~Texture2D() {
   if (m_Data) stbi_image_free(m_Data);
}

std::unique_ptr<Texture2D> Texture2D::Create(const char* filepath) {
   switch (Renderer::GetCurrentAPI()) {
      case GraphicsAPI::VULKAN: return std::make_unique<Texture2DVk>(filepath);
   }

   return nullptr;
}
