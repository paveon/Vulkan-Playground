#include "Texture.h"
#include <Platform/Vulkan/TextureVk.h>

#include <stb_image.h>
#include <cstring>
#include "RendererAPI.h"


Texture2D::Texture2D(unsigned char *data, uint32_t width, uint32_t height, uint32_t channels) : m_Width(width),
                                                                                                m_Height(height),
                                                                                                m_Channels(channels) {
    if (!data)
        throw std::runtime_error("invalid texture data!");

    auto byteSize = m_Width * m_Height * m_Channels;
    m_Data.resize(byteSize);
    memcpy(m_Data.data(), data, byteSize);
}


auto Texture2D::Create(const char *filepath) -> Texture2D * {
    static std::unordered_map<std::string, std::shared_ptr<Texture2D>> loadedTextures;
    auto it = loadedTextures.find(filepath);
    if (it != loadedTextures.end()) {
        return it->second.get();
    } else {
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


auto Texture2D::Create(unsigned char *data, uint32_t width, uint32_t height,
                       uint32_t channels) -> std::shared_ptr<Texture2D> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<Texture2DVk>(data, width, height, channels);
    }

    return nullptr;
}


auto TextureCubemap::Create(std::array<u_char *, 6> data, uint32_t width, uint32_t height, uint32_t channels)
-> std::shared_ptr<TextureCubemap> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<TextureCubemapVk>(data, width, height, channels);
    }

    return nullptr;

}


TextureCubemap::TextureCubemap(std::array<u_char *, 6> facesData, uint32_t width, uint32_t height, uint32_t channels)
        : m_Width(width), m_Height(height), m_Channels(channels), m_FaceBytes(width * height * channels) {

    m_Data.resize(m_FaceBytes * 6);
    size_t offset = 0;
    for (u_char *faceDataPtr : facesData) {
        memcpy(&m_Data[offset], faceDataPtr, m_FaceBytes);
        offset += m_FaceBytes;
    }
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
                for (size_t faceIdx = 0; faceIdx < i; faceIdx++)
                    stbi_image_free(faceData[faceIdx]);
                throw std::runtime_error("failed to load texture image!");
            }
        }

        it = loadedTextures.emplace(filepaths[0], Create(faceData, width, height, 4)).first;
        for (u_char *dataPtr : faceData)
            stbi_image_free(dataPtr);

        it->second->Upload();
        return it->second.get();
    }

//    unsigned int textureID;
//    glGenTextures(1, &textureID);
//    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
//
//    int width, height, nrChannels;
//    for (unsigned int i = 0; i < faces.size(); i++)
//    {
//        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
//        if (data)
//        {
//            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
//                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
//            );
//            stbi_image_free(data);
//        }
//        else
//        {
//            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
//            stbi_image_free(data);
//        }
//    }
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//
//    return textureID;
}
