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
    glm::mat4 projection;
    glm::mat4 viewModel;
    glm::mat4 viewNormalMatrix;
    glm::mat4 modelNormalMatrix;
};


//struct MaterialUBO {
//    glm::vec4 ambient;
//    glm::vec4 diffuse;
//    glm::vec4 specular;
//    float shininess;
//    int32_t diffuseTexIdx;
//    int32_t specularTexIdx;
//    int32_t emissionTexIdx;
//};


template<typename T>
struct TextureKey {
    uint64_t key;

    TextureKey(uint32_t set, uint32_t binding, T type) :
            key(((uint64_t) ((set << 16ul) + binding) << 32ul) + (uint32_t) type) {}

    TextureKey(BindingKey bindingKey, T type) :
            key(((uint64_t) bindingKey.value << 32ul) + (uint32_t) type) {}

    auto Set() const { return (key & 0xFFFF000000000000ul) >> 48u; }

    auto Binding() const { return (key & 0x0000FFFF00000000ul) >> 32u; }

    auto TextureType() const -> T { return (T) (key & 0x00000000FFFFFFFFul); }

    auto operator()() const -> std::size_t { return key; }

    auto operator==(const TextureKey &other) const -> bool { return key == other.key; }
};


namespace std {
    template<>
    struct hash<TextureKey<Texture2D::Type>> {
        auto operator()(const TextureKey<Texture2D::Type> &k) const -> std::size_t {
           return std::hash<uint64_t>()(k.key);
        }
    };

    template<>
    struct hash<TextureKey<TextureCubemap::Type>> {
        auto operator()(const TextureKey<TextureCubemap::Type> &k) const -> std::size_t {
           return std::hash<uint64_t>()(k.key);
        }
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

    auto GetMaterial() -> Material* { return m_Material; }

    auto GetMaterial() const -> const Material* { return m_Material; }

    template<typename T>
    void SetUniform(BindingKey bindingKey, const std::string &memberName, const T &value);

    template<typename T>
    auto GetUniform(BindingKey bindingKey, const std::string &memberName) -> T;
};


class Material {
    friend class MaterialInstance;

    struct BoundTexture2D {
        const Texture2D *texture{};
        uint32_t samplerIdx = 0;
    };

    struct BoundCubemap {
        const TextureCubemap *texture{};
        uint32_t samplerIdx = 0;
    };

    struct Uniform {
        std::vector<uint8_t> data;
        uint32_t objectSize;
        bool perObject;
    };

    std::string m_Name;

    std::shared_ptr<ShaderPipeline> m_ShaderPipeline;
    std::vector<uint8_t> m_VertexLayout;
    std::unordered_map<TextureKey<Texture2D::Type>, std::vector<BoundTexture2D>> m_BoundTextures2D;
    std::unordered_map<TextureKey<TextureCubemap::Type>, std::vector<BoundCubemap>> m_BoundCubemaps;
    std::unordered_map<BindingKey, Uniform> m_UniformData;
    std::unordered_map<BindingKey, Uniform> m_SharedUniformData;
//    std::vector<MaterialUBO> m_MaterialUBOs;

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

    Material(std::string name, std::shared_ptr<ShaderPipeline> shaderPipeline);

    ~Material() {
       if (m_ShaderPipeline) {
          m_ShaderPipeline->OnDetach(m_MaterialID);
       }
    }

    Material(const Material &other) = delete;

    auto operator=(const Material &other) -> Material & = delete;

    Material(Material &&other) noexcept = default;

    auto operator=(Material &&other) noexcept -> Material & = default;

    auto CreateInstance() -> MaterialInstance;

    auto GetInstanceCount() const { return m_InstanceCount; }

    void SetShaderPipeline(std::shared_ptr<ShaderPipeline> pipeline);

    auto GetPipeline() const -> ShaderPipeline & { return *m_ShaderPipeline; }

    auto GetMaterialID() const { return m_MaterialID; }

    auto GetBoundTextures2D() const ->
    const std::unordered_map<TextureKey<Texture2D::Type>, std::vector<BoundTexture2D>> & { return m_BoundTextures2D; }

    auto GetBoundCubemaps() const ->
    const std::unordered_map<TextureKey<TextureCubemap::Type>, std::vector<BoundCubemap>> & { return m_BoundCubemaps; }


    void BindUniformBuffer(const UniformBuffer *ub, BindingKey bindingKey) {
       m_ShaderPipeline->BindUniformBuffer(ub, bindingKey);
    }

//    auto BindTextures(const std::vector<std::pair<Texture2D::Type, const Texture2D *>> &textures,
//                      BindingKey bindingKey) -> std::vector<uint32_t>;

    auto BindTextures(const std::unordered_map<Texture2D::Type, const Texture2D *> &textures, BindingKey bindingKey)
    -> std::unordered_map<Texture2D::Type, uint32_t>;

    auto BindCubemaps(const std::unordered_map<TextureCubemap::Type, const TextureCubemap *> &textures,
                      BindingKey bindingKey) -> std::unordered_map<TextureCubemap::Type, uint32_t>;

    auto VertexLayout() const -> const auto & { return m_VertexLayout; }

//    void AllocateResources(uint32_t objectCount);

    template<typename T>
    void SetUniform(BindingKey bindingKey, const std::string &memberName, const T &value, bool propagate = false) {
       auto uniformIt = m_ShaderPipeline->ShaderUniforms().find(bindingKey);
       if (uniformIt == m_ShaderPipeline->ShaderUniforms().end()) {
          std::ostringstream msg;
          msg << "[Material::SetUniform] Shader '" << m_Name
              << "' doesn't have uniform binding {" << bindingKey.Set() << ";" << bindingKey.Binding() << "}";
          throw std::runtime_error(msg.str().c_str());
       }
       const auto &uniform = uniformIt->second;
       auto memberIt = uniform.members.find(memberName);
       if (memberIt == uniform.members.end()) {
          std::ostringstream msg;
          msg << "[Material::SetUniform] Uniform structure at binding {"
              << bindingKey.Set() << ";" << bindingKey.Binding() << "}"
              << "' doesn't have '" << memberName << "' member";
          throw std::runtime_error(msg.str().c_str());
       }
       const auto &member = memberIt->second;
       if (member.size != sizeof(T)) {
          std::ostringstream msg;
          msg << "[Material::SetUniform] Invalid size of input data: " << sizeof(T)
              << " bytes. Member '" << memberName << "' of uniform structure at binding {"
              << bindingKey.Set() << ";" << bindingKey.Binding() << "} has size: " << member.size << " bytes";
          throw std::runtime_error(msg.str().c_str());
       }

       auto dataIt = m_SharedUniformData.find(bindingKey);
       if (dataIt == m_SharedUniformData.end()) {
          std::ostringstream msg;
          msg << "[(" << m_Name << ")->SetUniform] Uniform {"
              << bindingKey.Set() << ";" << bindingKey.Binding() << "} doesn't exist";
          throw std::runtime_error(msg.str().c_str());
       }
       auto &sharedUniformData = dataIt->second.data;
       std::memcpy(&sharedUniformData[member.offset], &value, member.size);

       /* Propagate the change into material instances too if it's a per object uniform */
       if (propagate && uniform.perObject) {
          dataIt = m_UniformData.find(bindingKey);
          auto& uniformData = dataIt->second.data;
          for (size_t instanceID = 0; instanceID < m_InstanceCount; ++instanceID) {
             size_t instanceOffset = dataIt->second.objectSize * instanceID;
             std::memcpy(&uniformData[instanceOffset + member.offset], &value, member.size);
          }
       }
//        m_ShaderPipeline->SetUniformData(m_MaterialID, bindingKey, &uniformData[member.offset], 1);
    }


    template<typename T>
    auto RetrieveUniformMember(size_t instanceID, BindingKey bindingKey, const std::string &memberName)
    -> std::pair<const UniformMember*, Uniform*> {
       if (instanceID >= m_InstanceCount) {
          std::ostringstream msg;
          msg << "[(" << m_Name << ")->RetrieveUniformMember] Object index '" << m_InstanceCount << "' is out of bounds";
          throw std::runtime_error(msg.str().c_str());
       }

       auto uniformIt = m_ShaderPipeline->ShaderUniforms().find(bindingKey);
       if (uniformIt == m_ShaderPipeline->ShaderUniforms().end()) {
          std::ostringstream msg;
          msg << "[Material::RetrieveUniformMember] Shader '" << m_Name
              << "' doesn't have uniform binding {" << bindingKey.Set() << ";" << bindingKey.Binding() << "}";
          throw std::runtime_error(msg.str().c_str());
       }
       const auto &uniform = uniformIt->second;
       auto memberIt = uniform.members.find(memberName);
       if (memberIt == uniform.members.end()) {
          std::ostringstream msg;
          msg << "[Material::RetrieveUniformMember] Uniform structure at binding {"
              << bindingKey.Set() << ";" << bindingKey.Binding() << "}"
              << "' doesn't have '" << memberName << "' member";
          throw std::runtime_error(msg.str().c_str());
       }
       const auto &member = memberIt->second;
       if (member.size != sizeof(T)) {
          std::ostringstream msg;
          msg << "[Material::RetrieveUniformMember] Invalid size of input data: " << sizeof(T)
              << " bytes. Member '" << memberName << "' of uniform structure at binding {"
              << bindingKey.Set() << ";" << bindingKey.Binding() << "} has size: " << member.size << " bytes";
          throw std::runtime_error(msg.str().c_str());
       }

       auto uniformDataIt = m_UniformData.find(bindingKey);
       if (uniformDataIt == m_UniformData.end()) {
          std::ostringstream msg;
          msg << "[(" << m_Name << ")->SetInstanceUniform] Uniform {"
              << bindingKey.Set() << ";" << bindingKey.Binding() << "} doesn't exist";
          throw std::runtime_error(msg.str().c_str());
       }
       return {&member, &uniformDataIt->second};
    }


    template<typename T>
    void SetInstanceUniform(size_t instanceID, BindingKey bindingKey, const std::string &memberName, const T &value) {
       const auto& [member, uniformData] = RetrieveUniformMember<T>(instanceID, bindingKey, memberName);
       size_t instanceOffset = uniformData->objectSize * instanceID;
       std::memcpy(&uniformData->data[instanceOffset + member->offset], &value, member->size);
    }

    template<typename T>
    auto GetInstanceUniform(size_t instanceID, BindingKey bindingKey, const std::string &memberName) -> T {
       const auto& [member, uniformData] = RetrieveUniformMember<T>(instanceID, bindingKey, memberName);
       size_t instanceOffset = uniformData->objectSize * instanceID;
       T retrievedValue;
       std::memcpy(&retrievedValue, &uniformData->data[instanceOffset + member->offset], member->size);
       return retrievedValue;
    }

//    template<typename T>
//    void SetDynamicUniforms(BindingKey bindingKey, size_t firstIndex, const std::vector<T> &values) {
//       if ((firstIndex + values.size()) > m_InstanceCount) {
//          std::ostringstream msg;
//          msg << "[(" << m_Name << ")->SetDynamicUniforms] Object index '" << m_InstanceCount << "' is out of bounds";
//          throw std::runtime_error(msg.str().c_str());
//       }
//
//       m_ShaderPipeline->SetUniformData(m_MaterialID, bindingKey, values.data(), values.size());
//    }
//
//
//    template<typename T>
//    void SetDynamicUniform(BindingKey bindingKey, size_t index, const T &value) {
//       std::vector<T> values{value};
//       SetDynamicUniforms(bindingKey, index, values);
//    }


    void UpdateUniforms();
};

template<typename T>
void MaterialInstance::SetUniform(BindingKey bindingKey, const std::string &memberName, const T &value) {
   m_Material->SetInstanceUniform(m_InstanceID, bindingKey, memberName, value);
}

template<typename T>
T MaterialInstance::GetUniform(BindingKey bindingKey, const std::string &memberName) {
   return m_Material->GetInstanceUniform<T>(m_InstanceID, bindingKey, memberName);
}

#endif //GAME_ENGINE_MATERIAL_H
