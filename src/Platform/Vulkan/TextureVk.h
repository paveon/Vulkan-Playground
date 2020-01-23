#ifndef VULKAN_TEXTUREVK_H
#define VULKAN_TEXTUREVK_H

#include "Engine/Renderer/vulkan_wrappers.h"
#include "Engine/Renderer/Texture.h"

class Texture2DVk : public Texture2D {
private:
   vk::Image m_TextureImage;
   vk::DeviceMemory m_TextureMemory;
   vk::ImageView m_TextureView;

public:
   Texture2DVk(unsigned char* data, int width, int height, int channels = 4) :
           Texture2D(data, width, height, channels) {}

   void Upload() override;

   const vk::ImageView& View() const { return m_TextureView; }
};


#endif //VULKAN_TEXTUREVK_H
