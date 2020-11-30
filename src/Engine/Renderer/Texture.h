#ifndef VULKAN_TEXTURE_H
#define VULKAN_TEXTURE_H

#include <cstdint>
#include <tuple>
#include <cmath>
#include <memory>
#include <vector>


class Texture2D {
public:
    enum class Type {
        DIFFUSE,
        SPECULAR
    };

protected:
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_Channels = 0;
    std::vector<u_char> m_Data;

    Texture2D(u_char *data, uint32_t width, uint32_t height, uint32_t channels);

    static auto Create(u_char *data, uint32_t width, uint32_t height, uint32_t channels = 4) -> std::shared_ptr<Texture2D>;

public:
    virtual ~Texture2D() = default;

    auto Width() const -> uint32_t { return m_Width; }

    auto Height() const -> uint32_t { return m_Height; }

    auto Extent() const -> std::pair<uint32_t, uint32_t> {
        return {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)};
    }

    auto Size() const -> uint64_t { return m_Data.size(); }

    auto Data() const -> const u_char * { return m_Data.data(); }

    auto MipLevels() const -> uint32_t { return std::floor(std::log2(std::max(m_Width, m_Height))) + 1; }

    static auto Create(const char *filepath) -> Texture2D*;

    virtual void Upload() = 0;
};


class TextureCubemap {
protected:
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_Channels = 0;
    uint64_t m_FaceBytes = 0;
    std::vector<u_char> m_Data;

    TextureCubemap(std::array<u_char*, 6> facesData, uint32_t width, uint32_t height, uint32_t channels);

    static auto Create(std::array<u_char*, 6> data, uint32_t width, uint32_t height, uint32_t channels = 4) -> std::shared_ptr<TextureCubemap>;

public:
    virtual ~TextureCubemap() = default;

    auto Width() const -> uint32_t { return m_Width; }

    auto Height() const -> uint32_t { return m_Height; }

    auto Extent() const -> std::pair<uint32_t, uint32_t> {
        return {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)};
    }

    auto Size() const -> uint64_t { return m_Data.size(); }

    auto Data() const -> const u_char * { return m_Data.data(); }

    auto MipLevels() const -> uint32_t { return std::floor(std::log2(std::max(m_Width, m_Height))) + 1; }

    static auto Create(std::array<const char*, 6> filepaths) -> TextureCubemap*;

    virtual void Upload() = 0;
};


#endif //VULKAN_TEXTURE_H
