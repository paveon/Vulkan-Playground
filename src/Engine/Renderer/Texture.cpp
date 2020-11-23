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


auto Texture2D::Create(const char *filepath) -> Texture2D* {
    static std::unordered_map<std::string, std::shared_ptr<Texture2D>> loadedTextures;
    auto it = loadedTextures.find(filepath);
    if (it != loadedTextures.end()) {
        return it->second.get();
    }
    else {
        int width = 0, height = 0, channels = 0;

        stbi_set_flip_vertically_on_load(true);
        auto *tmp = stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
        if (!tmp)
            throw std::runtime_error("failed to load texture image!");

        it = loadedTextures.emplace(filepath, Create(tmp, width, height, 4)).first;
        stbi_image_free(tmp);

        it->second->Upload();
        return it->second.get();
    }
}


auto Texture2D::Create(unsigned char *data, int width, int height, int channels) -> std::shared_ptr<Texture2D> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<Texture2DVk>(data, width, height, channels);
    }

    return nullptr;
}
