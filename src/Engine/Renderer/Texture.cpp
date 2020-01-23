#include "Texture.h"
#include "renderer.h"
#include <stb_image.h>
#include <Platform/Vulkan/TextureVk.h>


Texture2D::Texture2D(unsigned char* data, int width, int height, int channels) : m_Width(width),
                                                                                 m_Height(height),
                                                                                 m_Channels(channels) {
   if (!data)
      throw std::runtime_error("invalid texture data!");

   auto size = m_Width * m_Height * m_Channels;
   m_Data.resize(size);
   memcpy(m_Data.data(), data, size);
}


std::unique_ptr<Texture2D> Texture2D::Create(const char* filepath) {
   int width, height, channels;
   auto tmp = stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
   if (!tmp)
      throw std::runtime_error("failed to load texture image!");

   auto texture = Create(tmp, width, height, 4);
   stbi_image_free(tmp);
   return texture;
}


std::unique_ptr<Texture2D> Texture2D::Create(unsigned char* data, int width, int height, int channels) {
   switch (Renderer::GetCurrentAPI()) {
      case GraphicsAPI::VULKAN:
         return std::make_unique<Texture2DVk>(data, width, height, channels);
   }

   return nullptr;
}
