#ifndef VULKAN_SHADERPROGRAM_H
#define VULKAN_SHADERPROGRAM_H


#include <memory>
#include <vector>

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

//struct DescriptorBinding {
//    DescriptorType type;
//    uint32_t count;
//};

//using VertexLayout = std::vector<VertexAttribute>;
//using DescriptorSetLayout = std::vector<DescriptorBinding>;


//struct VertexBinding {
//    VertexLayout vertexLayout;
//    uint32_t binding;
//    VertexBindingInputRate inputRate;
//};


class ShaderProgram {
public:
    struct UniformObject {
        std::string name;
        uint32_t key;
        uint32_t size;
        bool dynamic;
    };

protected:
    std::vector<uint8_t> m_VertexLayout;
    std::vector<UniformObject> m_UniformObjects;

    ShaderProgram() = default;

public:
    static auto Create(const std::vector<std::pair<const char *, ShaderType>> &shaders,
                       const std::vector<std::pair<uint32_t, uint32_t>>& dynamicOverrides)
    -> std::unique_ptr<ShaderProgram>;

    auto VertexLayout() const -> const auto & { return m_VertexLayout; }

    auto UniformObjects() const -> const auto & { return m_UniformObjects; }
};


#endif //VULKAN_SHADERPROGRAM_H
