#ifndef GAME_ENGINE_MATERIAL_H
#define GAME_ENGINE_MATERIAL_H

#include <memory>
#include <unordered_map>
#include <cstring>
#include <numeric>
#include <utility>
#include <glm/vec4.hpp>
#include <glm/glm.hpp>
#include "Texture.h"
#include "ShaderPipeline.h"


struct TransformUBO {
    glm::mat4 mvp;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 viewModel;
    glm::mat4 normalMatrix;
};


struct MaterialUBO {
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    float shininess;
    int32_t diffuseTexIdx;
    int32_t specularTexIdx;
    int32_t emissionTexIdx;
};


struct TextureKey {
    uint64_t key;

    TextureKey(uint32_t set, uint32_t binding, Texture2D::Type type) :
            key(((uint64_t) ((set << 16ul) + binding) << 32ul) + (uint32_t) type) {}

    TextureKey(BindingKey bindingKey, Texture2D::Type type) :
            key(((uint64_t) bindingKey.value << 32ul) + (uint32_t) type) {}

    auto Set() const { return (key & 0xFFFF000000000000ul) >> 48u; }

    auto Binding() const { return (key & 0x0000FFFF00000000ul) >> 32u; }

    auto TextureType() const -> Texture2D::Type { return (Texture2D::Type) (key & 0x00000000FFFFFFFFul); }

    auto operator()() const -> std::size_t { return key; }

    auto operator==(const TextureKey &other) const -> bool { return key == other.key; }
};


namespace std {
    template<>
    struct hash<TextureKey> {
        auto operator()(const TextureKey &k) const -> std::size_t { return std::hash<uint64_t>()(k.key); }
    };
}


class Material;

class MaterialInstance {
    friend class Material;

    Material *m_Material{};
    uint32_t m_InstanceID{};

    MaterialInstance(Material *model, uint32_t id) : m_Material(model), m_InstanceID(id) {}

    void Move(MaterialInstance &other) {
        m_Material = other.m_Material;
        m_InstanceID = other.m_InstanceID;
        other.m_Material = nullptr;
        other.m_InstanceID = 0;
    }

public:
    MaterialInstance() = default;

    MaterialInstance(const MaterialInstance &other) = delete;

    auto operator=(const MaterialInstance &other) -> MaterialInstance & = delete;

    MaterialInstance(MaterialInstance &&other) noexcept { Move(other); }

    auto operator=(MaterialInstance &&other) noexcept -> MaterialInstance & {
        Move(other);
        return *this;
    };

    auto InstanceID() const { return m_InstanceID; }

    auto GetMaterial() const -> const Material * { return m_Material; }
};


class Material {
    friend class MaterialInstance;

    struct BoundTexture {
        const Texture2D *texture{};
        uint32_t samplerIdx = 0;
    };

    struct Uniform {
        std::vector<uint8_t> data;
        uint32_t objectSize;
        bool dynamic;
    };

    std::string m_Name;

    std::shared_ptr<ShaderPipeline> m_ShaderPipeline;
    std::vector<uint8_t> m_VertexLayout;
    std::unordered_map<BindingKey, Uniform> m_UniformData;

    std::unordered_map<TextureKey, std::vector<BoundTexture>> m_BoundTextures;
    std::vector<MaterialUBO> m_MaterialUBOs;

    BindingKey m_MaterialBinding;
    uint32_t m_MaterialID = 0;
    uint32_t m_InstanceCount = 0;
    //    uint32_t m_ObjectCount = 0;

//    void Move(Material &other) {
//        m_ShaderPipeline = std::move(other.m_ShaderPipeline);
//        m_VertexLayout = std::move(other.m_VertexLayout);
//        m_BoundTextures = std::move(other.m_BoundTextures);
//        m_UniformData = std::move(other.m_UniformData);
//        m_BoundTextures2 = std::move(other.m_BoundTextures2);
//        m_Name = std::move(other.m_Name);
//    }

public:
    explicit Material(std::string name);

    Material(std::string name, std::shared_ptr<ShaderPipeline> shaderPipeline, BindingKey materialSlot);

    ~Material() {
        if (m_ShaderPipeline) {
            m_ShaderPipeline->OnDetach(m_MaterialID);
        }
    }

    Material(const Material &other) = delete;

    auto operator=(const Material &other) -> Material & = delete;

    Material(Material &&other) noexcept = default;

    auto operator=(Material &&other) noexcept -> Material & = default;

    auto CreateInstance(BindingKey) -> MaterialInstance;

    auto GetInstanceCount() const { return m_InstanceCount; }

    void SetShaderPipeline(std::shared_ptr<ShaderPipeline> pipeline, BindingKey materialSlot);

    auto GetPipeline() const -> ShaderPipeline & { return *m_ShaderPipeline; }

    auto GetMaterialID() const { return m_MaterialID; }

    void BindUniformBuffer(const UniformBuffer *ub, BindingKey bindingKey) {
        m_ShaderPipeline->BindUniformBuffer(ub, bindingKey);
    }

    void BindTextures(const std::vector<std::pair<Texture2D::Type, const Texture2D *>> &textures,
                      BindingKey bindingKey);

    auto VertexLayout() const -> const auto & { return m_VertexLayout; }

    auto MaterialBinding() const  { return m_MaterialBinding; }

//    void AllocateResources(uint32_t objectCount);

    template<typename T>
    void SetDynamicUniforms(BindingKey bindingKey, size_t firstIndex, const std::vector<T> &values) {
        if ((firstIndex + values.size()) > m_InstanceCount) {
            std::ostringstream msg;
            msg << "[(" << m_Name << ")->SetDynamicUniforms] Object index '" << m_InstanceCount << "' is out of bounds";
            throw std::runtime_error(msg.str().c_str());
        }

        auto it = m_UniformData.find(bindingKey);
        if (it == m_UniformData.end()) {
            std::ostringstream msg;
            msg << "[(" << m_Name << ")->SetDynamicUniforms] Uniform {"
                << bindingKey.Set() << ";" << bindingKey.Binding() << "} doesn't exist";
            throw std::runtime_error(msg.str().c_str());
        }

        auto &uniform = it->second;
        if (uniform.objectSize != sizeof(T)) {
            std::ostringstream msg;
            msg << "[(" << m_Name << ")->SetDynamicUniforms] Invalid size of {"
                << bindingKey.Set() << ";" << bindingKey.Binding()
                << "} uniform data: " << sizeof(T) << " bytes. Expected: " << uniform.objectSize << " bytes";
            throw std::runtime_error(msg.str().c_str());
        }

        size_t offset = sizeof(T) * firstIndex;
        size_t dataSize = (sizeof(T) * values.size());
        if (uniform.data.size() < offset + dataSize) {
            uniform.data.resize(offset + dataSize);
        }
        std::memcpy(&uniform.data[offset], values.data(), dataSize);
    }


    template<typename T>
    void SetDynamicUniform(BindingKey bindingKey, size_t index, const T &value) {
        std::vector<T> values{value};
        SetDynamicUniforms(bindingKey, index, values);
    }


    void UpdateUniforms();
};


#endif //GAME_ENGINE_MATERIAL_H
