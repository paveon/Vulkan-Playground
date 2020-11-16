#include "Texture.h"
#include <Platform/Vulkan/TextureVk.h>

#include <stb_image.h>
#include <cstring>
#include "RendererAPI.h"


Texture2D::Texture2D(unsigned char *data, int width, int height, int channels) : m_Width(width),
                                                                                 m_Height(height),
                                                                                 m_Channels(channels) {
    if (!data)
        throw std::runtime_error("invalid texture data!");

    auto size = m_Width * m_Height * m_Channels;
    m_Data.resize(size);
    memcpy(m_Data.data(), data, size);
}


auto Texture2D::Create(const char *filepath) -> std::unique_ptr<Texture2D> {
    int width = 0, height = 0, channels = 0;

    stbi_set_flip_vertically_on_load(true);
    auto *tmp = stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
    if (!tmp)
        throw std::runtime_error("failed to load texture image!");

    auto texture = Create(tmp, width, height, 4);
    stbi_image_free(tmp);
    return texture;
}


auto Texture2D::Create(unsigned char *data, int width, int height, int channels) -> std::unique_ptr<Texture2D> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<Texture2DVk>(data, width, height, channels);
    }

    return nullptr;
}
