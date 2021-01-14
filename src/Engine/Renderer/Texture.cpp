#include "Texture.h"
#include <Platform/Vulkan/TextureVk.h>

#include <stb_image.h>
#include <cstring>
#include "RendererAPI.h"


Texture2D::Texture2D(const u_char *data, uint32_t width, uint32_t height, uint32_t channels, VkFormat format) :
        m_Width(width), m_Height(height), m_Channels(channels), m_Format(format) {

    size_t byteSize = m_Width * m_Height * m_Channels;
    m_Data.resize(byteSize);
    std::memcpy(m_Data.data(), data, byteSize);
}


auto Texture2D::Create(const char *filepath, VkFormat format, bool flipOnLoad) -> Texture2D * {
    static std::unordered_map<std::string, std::shared_ptr<Texture2D>> loadedTextures;
    auto it = loadedTextures.find(filepath);
    if (it != loadedTextures.end()) {
        return it->second.get();
    } else {
        int width = 0, height = 0, channels = 0;

        stbi_set_flip_vertically_on_load(flipOnLoad);
        auto *tmp = stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
        if (!tmp)
            throw std::runtime_error("failed to load texture image!");

        it = loadedTextures.emplace(filepath, Create(tmp, width, height, 4, format)).first;
        stbi_image_free(tmp);

        it->second->Upload();
        return it->second.get();
    }
}


auto Texture2D::Create(const u_char *data,
                       uint32_t width,
                       uint32_t height,
                       uint32_t channels,
                       VkFormat format) -> std::shared_ptr<Texture2D> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<Texture2DVk>(data, width, height, channels, format);
    }

    return nullptr;
}


auto Texture2D::GenerateBrdfLut(uint32_t resolution) -> std::unique_ptr<Texture2D> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return Texture2DVk::GenerateBrdfLut(resolution);
    }

    return nullptr;
}


auto TextureCubemap::Create(const std::array<u_char *, 6> &data, uint32_t width, uint32_t height, uint32_t channels)
-> std::shared_ptr<TextureCubemap> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<TextureCubemapVk>(data, width, height, channels);
    }

    return nullptr;

}


auto TextureCubemap::Create(const float *data,
                            uint32_t width,
                            uint32_t height,
                            uint32_t channels,
                            uint32_t faceResolution)
-> std::shared_ptr<TextureCubemap> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<TextureCubemapVk>(data, width, height, channels, faceResolution);
    }

    return nullptr;
}


auto TextureCubemap::Create(const u_char *data, uint32_t width, uint32_t height, uint32_t channels)
-> std::shared_ptr<TextureCubemap> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<TextureCubemapVk>(data, width, height, channels);
    }

    return nullptr;
}


TextureCubemap::TextureCubemap(const std::array<u_char *, 6> &facesData, uint32_t width, uint32_t height,
                               uint32_t channels)
        : m_Width(width), m_Height(height), m_Channels(channels), m_FaceBytes(width * height * channels) {

    m_Data.resize(m_FaceBytes * 6);
    size_t offset = 0;
    for (u_char *faceDataPtr : facesData) {
        std::memcpy(&m_Data[offset], faceDataPtr, m_FaceBytes);
        offset += m_FaceBytes;
    }
}


TextureCubemap::TextureCubemap(const float *data, uint32_t width, uint32_t height, uint32_t channels)
        : m_Width(width),
          m_Height(height),
          m_Channels(channels),
          m_FaceBytes(width * height * channels * 4) {

    m_Data.resize(m_FaceBytes);
    std::memcpy(m_Data.data(), data, m_FaceBytes);
}


TextureCubemap::TextureCubemap(const u_char *data, uint32_t width, uint32_t height, uint32_t channels)
        : m_Width(width),
          m_Height(height),
          m_Channels(channels),
          m_FaceBytes(width * height * channels) {

    m_Data.resize(m_FaceBytes);
    std::memcpy(m_Data.data(), data, m_FaceBytes);
}


auto TextureCubemap::Create(std::array<const char *, 6> filepaths) -> TextureCubemap * {
    static std::unordered_map<std::string, std::shared_ptr<TextureCubemap>> loadedTextures;
    auto it = loadedTextures.find(filepaths[0]);
    if (it != loadedTextures.end()) {
        return it->second.get();
    } else {
        int width = 0, height = 0, channels = 0;

        std::array<u_char *, 6> faceData{};
        stbi_set_flip_vertically_on_load(false);
        for (size_t i = 0; i < 6; i++) {
            faceData[i] = stbi_load(filepaths[i], &width, &height, &channels, STBI_rgb_alpha);
            if (!faceData[i]) {
                for (size_t faceIdx = 0; faceIdx < i; faceIdx++) {
                    stbi_image_free(faceData[faceIdx]);
                }

                throw std::runtime_error("failed to load texture image!");
            }
        }

        it = loadedTextures.emplace(filepaths[0], Create(faceData, width, height, STBI_rgb_alpha)).first;
        for (u_char *dataPtr : faceData)
            stbi_image_free(dataPtr);

        it->second->Upload();
        return it->second.get();
    }
}

auto TextureCubemap::CreateFromHDR(const std::string &hdrPath, uint32_t faceResolution) -> TextureCubemap * {
    static std::unordered_map<std::string, std::shared_ptr<TextureCubemap>> loadedTextures;
    auto it = loadedTextures.find(hdrPath);
    if (it != loadedTextures.end()) {
        return it->second.get();
    } else {
        stbi_set_flip_vertically_on_load(true);
        int width = 0, height = 0, channels = 0;
        float *tmp = stbi_loadf(hdrPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!tmp)
            throw std::runtime_error("failed to load texture image!");

        it = loadedTextures.emplace(hdrPath, Create(tmp, width, height, STBI_rgb_alpha, faceResolution)).first;
        stbi_image_free(tmp);

        it->second->HDRtoCubemap();
        return it->second.get();
    }
}
