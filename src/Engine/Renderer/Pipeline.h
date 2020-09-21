#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include <memory>
#include <vector>
#include "vulkan_wrappers.h"


enum class ShaderType {
    Float,
    Float2,
    Float3,
    Float4,
    UInt
};


enum class DescriptorType {
    UniformBuffer,
    Texture
};


inline uint32_t ShaderTypeSize(ShaderType type) {
    switch (type) {
        case ShaderType::Float:
        case ShaderType::UInt:
            return 4;
        case ShaderType::Float2:
            return 4 * 2;
        case ShaderType::Float3:
            return 4 * 3;
        case ShaderType::Float4:
            return 4 * 4;
    }

    throw std::runtime_error("Unknown shader data type");
}


struct PushConstant {
    uint32_t size;
};


struct VertexAttribute {
    ShaderType format;

    explicit VertexAttribute(ShaderType format) : format(format) {}
};

struct DescriptorBinding {
    DescriptorType type;

    explicit DescriptorBinding(DescriptorType type) : type(type) {}
};


using VertexLayout = std::vector<VertexAttribute>;
using DescriptorLayout = std::vector<DescriptorBinding>;

class RenderPass;

class ShaderProgram;

class Texture2D;

class UniformBuffer;

class Pipeline {
protected:
    Pipeline() = default;

public:
    virtual ~Pipeline() = default;

    virtual void Recreate(const RenderPass&) {}

    virtual void Bind(VkCommandBuffer cmdBuffer, uint32_t imageIndex) const = 0;

    virtual void BindTexture(const Texture2D &texture, uint32_t binding) const = 0;

    virtual void BindUniformBuffer(const UniformBuffer &buffer, uint32_t binding) const = 0;

    virtual void PushConstants(VkCommandBuffer cmdBuffer, uint32_t offset, uint32_t size, const void *data) const = 0;

    static auto Create(const RenderPass &renderPass,
                       const ShaderProgram &vertexShader,
                       const ShaderProgram &fragShader,
                       const VertexLayout &vertexLayout,
                       const DescriptorLayout &descriptorLayout,
                       const std::vector<PushConstant> &pushConstants,
                       bool enableDepthTest) -> std::unique_ptr<Pipeline>;
};


#endif //VULKAN_PIPELINE_H
