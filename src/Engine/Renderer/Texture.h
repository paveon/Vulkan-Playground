#ifndef VULKAN_TEXTURE_H
#define VULKAN_TEXTURE_H

#include <cstdint>
#include <tuple>
#include <cmath>
#include <memory>

class Texture2D {
private:
    int m_Width = 0;
    int m_Height = 0;
    int m_Channels = 0;
    uint64_t m_Size = 0;
    unsigned char* m_Data = nullptr;

protected:
    explicit Texture2D(const char* filepath);

public:
    ~Texture2D();

    uint32_t Width() const { return m_Width; }

    uint32_t Height() const { return m_Height; }

    std::pair<uint32_t, uint32_t> Extent() const {
       return {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)};
    }

    uint64_t Size() const { return m_Size; }

    const unsigned char* Data() const { return m_Data; }

    uint32_t MipLevels() const { return std::floor(std::log2(std::max(m_Width, m_Height))) + 1; }

    static std::unique_ptr<Texture2D> Create(const char* filepath);
};


#endif //VULKAN_TEXTURE_H
