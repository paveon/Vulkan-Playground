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
    int m_Width = 0;
    int m_Height = 0;
    int m_Channels = 0;
    std::vector<unsigned char> m_Data;

    Texture2D(unsigned char *data, int width, int height, int channels);

    static auto Create(unsigned char *data, int width, int height, int channels = 4) -> std::shared_ptr<Texture2D>;

public:
    virtual ~Texture2D() = default;

    auto Width() const -> uint32_t { return m_Width; }

    auto Height() const -> uint32_t { return m_Height; }

    auto Extent() const -> std::pair<uint32_t, uint32_t> {
        return {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)};
    }

    auto Size() const -> uint64_t { return m_Data.size(); }

    auto Data() const -> const unsigned char * { return m_Data.data(); }

    auto MipLevels() const -> uint32_t { return std::floor(std::log2(std::max(m_Width, m_Height))) + 1; }

    static auto Create(const char *filepath) -> Texture2D*;

    virtual void Upload() {}
};


#endif //VULKAN_TEXTURE_H
