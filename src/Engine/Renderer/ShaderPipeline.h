#ifndef VULKAN_SHADERPROGRAM_H
#define VULKAN_SHADERPROGRAM_H

#include <memory>
#include <vector>
#include "vulkan_wrappers.h"


struct BindingKey {
    uint32_t value = 0;

    BindingKey() = default;

    BindingKey(uint32_t value) : value(value) {}

    BindingKey(uint32_t set, uint32_t binding) : value((set << 16ul) + binding) {}

    auto Set() const { return (value & 0xFFFF0000u) >> 16u; }

    auto Binding() const { return value & 0x0000FFFFu; }

    auto operator()() const -> std::size_t { return value; }

    auto operator==(const BindingKey &other) const -> bool { return value == other.value; }
};


struct MaterialKey {
    uint64_t value;

    MaterialKey(uint32_t set, uint32_t binding, uint32_t materialID) :
            value(((uint64_t) ((set << 16ul) + binding) << 32ul) + materialID) {}

    MaterialKey(BindingKey bindingKey, uint32_t materialID) :
            value(((uint64_t) bindingKey.value << 32ul) + materialID) {}

    auto Set() const { return (value & 0xFFFF000000000000ul) >> 48u; }

    auto Binding() const { return (value & 0x0000FFFF00000000ul) >> 32u; }

    auto MaterialID() const -> uint32_t { return value & 0x00000000FFFFFFFFul; }

    auto operator()() const -> std::size_t { return value; }

    auto operator==(const MaterialKey &other) const -> bool { return value == other.value; }
};


namespace std {
    template<>
    struct hash<BindingKey> {
        auto operator()(const BindingKey &k) const -> std::size_t { return std::hash<uint32_t>()(k.value); }
    };

    template<>
    struct hash<MaterialKey> {
        auto operator()(const MaterialKey &k) const -> std::size_t { return std::hash<uint64_t>()(k.value); }
    };
}


enum class ShaderType {
    VERTEX_SHADER,
    GEOMETRY_SHADER,
    TESSELATION_SHADER,
    FRAGMENT_SHADER,
    COMPUTE_SHADER
};

inline auto ShaderTypeToString(ShaderType type) -> const char * {
    switch (type) {
        case ShaderType::VERTEX_SHADER:
            return "Vertex Shader";
        case ShaderType::FRAGMENT_SHADER:
            return "Fragment Shader";
        case ShaderType::GEOMETRY_SHADER:
            return "Geometry Shader";
        case ShaderType::TESSELATION_SHADER:
            return "Tesselation Shader";
        case ShaderType::COMPUTE_SHADER:
            return "Compute Shader";
    }
    return "Unknown Shader Type";
}

enum class ShaderAttributeType {
    Float,
    Float2,
    Float3,
    Float4,
    UInt
};

enum class VertexBindingInputRate {
    PER_VERTEX = 0,
    PER_INSTANCE = 1,
};

enum class DescriptorType {
    UniformBuffer,
    UniformBufferDynamic,
    Texture
};


struct VertexAttribute {
    ShaderAttributeType format;
};


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

class ShaderPipeline;

class Texture2D;

class UniformBuffer;

class Material;

class ShaderPipeline {
public:
    struct Uniform {
        std::string name;
        uint32_t size;
        bool perObject;
    };

protected:
    std::string m_Name;

    std::vector<uint8_t> m_VertexLayout;
    std::unordered_map<BindingKey, Uniform> m_ShaderUniforms;

    std::unordered_map<uint32_t, const Material *> m_BoundMaterials;
    uint32_t m_MaterialCount = 0;

    explicit ShaderPipeline(std::string name) : m_Name(std::move(name)) {};

public:
    virtual ~ShaderPipeline() = default;

    static auto Create(std::string name,
                       const std::vector<std::pair<const char *, ShaderType>> &shaders,
                       const std::vector<BindingKey> &perObjectUniforms,
                       const RenderPass &renderPass,
                       uint32_t subpassIndex,
                       std::pair<VkCullModeFlags, VkFrontFace> culling,
                       bool enableDepthTest) -> std::unique_ptr<ShaderPipeline>;

    auto OnAttach(const Material *material) -> uint32_t {
        auto it = std::find_if(m_BoundMaterials.begin(), m_BoundMaterials.end(), [material](const auto &kv) {
            return kv.second == material;
        });
        if (it == m_BoundMaterials.end()) {
            m_BoundMaterials.insert({m_MaterialCount, material});
            return m_MaterialCount++;
        } else {
            return it->first;
        }
    }

    void OnDetach(uint32_t materialID) {
        auto it = m_BoundMaterials.find(materialID);
        if (it != m_BoundMaterials.end()) {
            m_MaterialCount--;
            m_BoundMaterials.erase(it);
        }
    }

    auto VertexLayout() const -> const auto & { return m_VertexLayout; }

    auto ShaderUniforms() const -> const auto & { return m_ShaderUniforms; }

    virtual void Recreate(const RenderPass &) {}

    virtual void AllocateResources() = 0;

//    virtual void Bind(VkCommandBuffer cmdBuffer, uint32_t imageIndex) = 0;

    virtual auto BindTextures(const std::vector<const Texture2D *> &textures, BindingKey bindingKey) -> std::vector<uint32_t> = 0;

    virtual void BindUniformBuffer(const UniformBuffer *buffer, BindingKey bindingKey) = 0;

//    virtual void PushConstants(VkCommandBuffer cmdBuffer, uint32_t offset, uint32_t size, const void *data) const = 0;

    virtual void SetDynamicOffsets(uint32_t set, const std::vector<uint32_t> &dynamicOffsets) const = 0;

//    virtual void SetDynamicOffsets(uint32_t objectIndex) const = 0;

    virtual void SetUniformData(uint32_t materialID, BindingKey bindingKey, const void *objectData, size_t objectCount) = 0;
};


#endif //VULKAN_SHADERPROGRAM_H
