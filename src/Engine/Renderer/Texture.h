#ifndef VULKAN_TEXTURE_H
#define VULKAN_TEXTURE_H

#include <cstdint>
#include <tuple>
#include <cmath>
#include <memory>
#include <vector>
#include <vulkan_core.h>


class Texture2D {
public:
    enum class Type {
        DIFFUSE,
        SPECULAR,
        NORMAL,
        ALBEDO,
        METALLIC,
        ROUGHNESS,
        AMBIENT_OCCLUSION,
        BRDF_LUT
    };

protected:
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_Channels = 0;
    VkFormat m_Format;
    std::vector<u_char> m_Data;

    Texture2D(const u_char *data, uint32_t width, uint32_t height, uint32_t channels, VkFormat format);

    Texture2D(uint32_t width, uint32_t height, uint32_t channels, VkFormat format)
            : m_Width(width), m_Height(height), m_Channels(channels), m_Format(format) {}

    static auto Create(const u_char *data, uint32_t width, uint32_t height, uint32_t channels,
                       VkFormat format) -> std::shared_ptr<Texture2D>;

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

    static auto Create(const char *filepath, VkFormat format, bool flipOnLoad) -> Texture2D *;

    static auto GenerateBrdfLut(uint32_t resolution) -> std::unique_ptr<Texture2D>;

    virtual void Upload() = 0;
};


class TextureCubemap {
public:
    enum class Type {
        ENVIRONMENT,
        IRRADIANCE,
        PREFILTERED_ENV
    };

protected:
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_Channels = 0;
    uint64_t m_FaceBytes = 0;
    std::vector<u_char> m_Data;

    TextureCubemap(const std::array<u_char *, 6> &facesData, uint32_t width, uint32_t height, uint32_t channels);

    TextureCubemap(const float *data, uint32_t width, uint32_t height, uint32_t channels);

    TextureCubemap(const u_char *data, uint32_t width, uint32_t height, uint32_t channels);

    TextureCubemap(uint32_t width, uint32_t height) : m_Width(width), m_Height(height) {}

    static auto Create(const std::array<u_char *, 6> &data,
                       uint32_t width,
                       uint32_t height,
                       uint32_t channels) -> std::shared_ptr<TextureCubemap>;

    static auto Create(const float *data,
                       uint32_t width,
                       uint32_t height,
                       uint32_t channels,
                       uint32_t faceResolution) -> std::shared_ptr<TextureCubemap>;

    static auto Create(const u_char *data,
                       uint32_t width,
                       uint32_t height,
                       uint32_t channels) -> std::shared_ptr<TextureCubemap>;

    virtual void HDRtoCubemap() = 0;

public:
    virtual ~TextureCubemap() = default;

    auto Width() const -> uint32_t { return m_Width; }

    auto Height() const -> uint32_t { return m_Height; }

    auto Extent() const -> std::pair<uint32_t, uint32_t> {
        return {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)};
    }

    auto Size() const -> uint64_t { return m_Data.size(); }

    auto Data() const -> const u_char * { return m_Data.data(); }

    static auto MipLevels(uint32_t width, uint32_t height) -> uint32_t {
        return std::floor(std::log2(std::max(width, height))) + 1;
    }

    static auto Create(std::array<const char *, 6> filepaths) -> TextureCubemap *;

    static auto CreateFromHDR(const std::string &hdrPath, uint32_t faceResolution) -> TextureCubemap *;

    virtual void Upload() = 0;

    virtual auto CreateIrradianceCubemap(uint32_t resolution) -> TextureCubemap * = 0;

    virtual auto CreatePrefilteredCubemap(uint32_t baseResolution, uint32_t maxMipLevels) -> TextureCubemap * = 0;
};


#endif //VULKAN_TEXTURE_H
