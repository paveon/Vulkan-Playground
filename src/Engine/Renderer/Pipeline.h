#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include <memory>
#include <vector>
#include "ShaderProgram.h"
#include "vulkan_wrappers.h"


inline auto ShaderTypeSize(ShaderAttributeType type) -> uint32_t {
    switch (type) {
        case ShaderAttributeType::Float:
        case ShaderAttributeType::UInt:
            return sizeof(float);
        case ShaderAttributeType::Float2:
            return sizeof(float) * 2;
        case ShaderAttributeType::Float3:
            return sizeof(float) * 3;
        case ShaderAttributeType::Float4:
            return sizeof(float) * 4;
    }

    throw std::runtime_error("[ShaderTypeSize] Unknown shader data type");
}


struct PushConstant {
    uint32_t size;
};


class RenderPass;

class ShaderProgram;

class Texture2D;

class UniformBuffer;

class Pipeline {
protected:
    Pipeline() = default;

public:
    virtual ~Pipeline() = default;

    virtual void Recreate(const RenderPass &) {}

    virtual void AllocateResources(uint32_t objectCount) = 0;

    virtual void Bind(VkCommandBuffer cmdBuffer, uint32_t imageIndex) = 0;

    virtual void BindTextures(const std::vector<const Texture2D *> &textures, uint32_t set, uint32_t binding) const = 0;

    virtual void BindUniformBuffer(const UniformBuffer &buffer, uint32_t set, uint32_t binding) const = 0;

    virtual void PushConstants(VkCommandBuffer cmdBuffer, uint32_t offset, uint32_t size, const void *data) const = 0;

    virtual void SetDynamicOffsets(uint32_t set, const std::vector<uint32_t> &dynamicOffsets) const = 0;

    virtual void SetDynamicOffsets(uint32_t objectIndex) const = 0;

    virtual void SetUniformData(uint32_t key, const void *objectData, size_t objectCount) = 0;

    static auto Create(const RenderPass &renderPass,
                       const ShaderProgram &program,
//                       const ShaderProgram &fragShader,
//                       const std::vector<VertexBinding> &vertexBindings,
//                       const std::vector<DescriptorSetLayout> &descriptorSetLayouts,
                       const std::vector<PushConstant> &pushConstants,
                       bool enableDepthTest) -> std::unique_ptr<Pipeline>;
};


#endif //VULKAN_PIPELINE_H
