#ifndef VULKAN_TEXTUREVK_H
#define VULKAN_TEXTUREVK_H

#include "Engine/Renderer/vulkan_wrappers.h"
#include "Engine/Renderer/Texture.h"

class Texture2DVk : public Texture2D {
private:
    vk::Image m_TextureImage;
    vk::DeviceMemory m_TextureMemory;

public:
    explicit Texture2DVk(const char* filepath) : Texture2D(filepath) {}
};


#endif //VULKAN_TEXTUREVK_H
