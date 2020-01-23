#ifndef VULKAN_TEXTURE_H
#define VULKAN_TEXTURE_H

#include <cstdint>
#include <tuple>
#include <cmath>
#include <memory>
#include <vector>


class Texture2D {
protected:
   int m_Width = 0;
   int m_Height = 0;
   int m_Channels = 0;
   std::vector<unsigned char> m_Data;

   Texture2D(unsigned char* data, int width, int height, int channels);

public:
   uint32_t Width() const { return m_Width; }

   uint32_t Height() const { return m_Height; }

   std::pair<uint32_t, uint32_t> Extent() const {
      return {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)};
   }

   uint64_t Size() const { return m_Data.size(); }

   const unsigned char* Data() const { return m_Data.data(); }

   uint32_t MipLevels() const { return std::floor(std::log2(std::max(m_Width, m_Height))) + 1; }

   static std::unique_ptr<Texture2D> Create(const char* filepath);

   static std::unique_ptr<Texture2D> Create(unsigned char* data, int width, int height, int channels = 4);

   virtual void Upload() {}
};


#endif //VULKAN_TEXTURE_H
