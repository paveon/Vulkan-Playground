#ifndef GAME_ENGINE_MATERIAL_H
#define GAME_ENGINE_MATERIAL_H

#include <memory>
#include <unordered_map>
#include <cstring>
#include <numeric>
#include <glm/vec4.hpp>
#include <glm/glm.hpp>
#include "Pipeline.h"


struct TransformUBO {
    glm::mat4 mvp;
    glm::mat4 viewModel;
    glm::mat4 normalMatrix;
//    glm::vec3 objectColor;
//    int32_t materialIdx;
};


struct MaterialUBO {
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    float shininess;
    uint32_t diffuseTexIdx;
    uint32_t specularTexIdx;
    uint32_t emissionTexIdx;
};


class Material {
private:
    struct Uniform {
        std::vector<uint8_t> data;
        uint32_t objectSize;
        bool dynamic;
    };

    std::unique_ptr<Pipeline> m_Pipeline;
    std::vector<uint8_t> m_VertexLayout;
    std::vector<const Texture2D *> m_BoundTextures;
    std::unordered_map<uint32_t, Uniform> m_UniformData;

    std::string m_Name;
    uint32_t m_ObjectCount = 0;

public:
//    size_t m_ResourceID = std::numeric_limits<size_t>::max();

    explicit Material(std::string name, const std::shared_ptr<ShaderProgram> &program);

    auto GetPipeline() const -> Pipeline & { return *m_Pipeline; }

    void BindUniformBuffer(const UniformBuffer &ub, uint32_t set, uint32_t binding) {
        m_Pipeline->BindUniformBuffer(ub, set, binding);
    }

    auto BindTextures(const std::vector<const Texture2D *> &textures,
                      uint32_t set,
                      uint32_t binding) -> std::vector<uint32_t> {
        auto textureCount = m_BoundTextures.size();
        m_BoundTextures.reserve(textureCount + textures.size());
        m_BoundTextures.insert(m_BoundTextures.end(), textures.begin(), textures.end());
        m_Pipeline->BindTextures(m_BoundTextures, set, binding);
        std::vector<uint32_t> indices(textures.size());
        std::iota(indices.begin(), indices.end(), textureCount);
        return indices;
    }

    auto OnAttach() -> auto { return m_ObjectCount++; };

    void OnDetach() { m_ObjectCount--; };

    auto VertexLayout() const -> const auto & { return m_VertexLayout; }

    void AllocateResources(uint32_t objectCount);

    template<typename T>
    void SetUniform(uint32_t set, uint32_t binding, const T &value) {
        auto it = m_UniformData.find((set << 16u) + binding);
        if (it == m_UniformData.end()) {
            std::ostringstream msg;
            msg << "[(" << m_Name << ")->SetUniform] Uniform {" << set << ";" << binding << "} doesn't exist";
            throw std::runtime_error(msg.str().c_str());
        }

        auto &uniform = it->second;
        if (uniform.objectSize != sizeof(T)) {
            std::ostringstream msg;
            msg << "[(" << m_Name << ")->SetUniform] Invalid size of {" << set << ";" << binding
                << "} uniform data: " << sizeof(T) << " bytes. Expected: " << uniform.objectSize << " bytes";
            throw std::runtime_error(msg.str().c_str());
        }

        std::memcpy(uniform.data.data(), &value, sizeof(T));
    }

    template<typename T>
    void SetDynamicUniform(uint32_t set, uint32_t binding, size_t index, const T &value) {
        if (index > m_ObjectCount) {
            std::ostringstream msg;
            msg << "[(" << m_Name << ")->SetDynamicUniform] Object index '" << index << "' is out of bounds";
            throw std::runtime_error(msg.str().c_str());
        }

        auto it = m_UniformData.find((set << 16u) + binding);
        if (it == m_UniformData.end()) {
            std::ostringstream msg;
            msg << "[(" << m_Name << ")->SetDynamicUniform] Uniform {" << set << ";" << binding << "} doesn't exist";
            throw std::runtime_error(msg.str().c_str());
        }

        auto &uniform = it->second;
        if (uniform.objectSize != sizeof(T)) {
            std::ostringstream msg;
            msg << "[(" << m_Name << ")->SetDynamicUniform] Invalid size of {" << set << ";" << binding
                << "} uniform data: " << sizeof(T) << " bytes. Expected: " << uniform.objectSize << " bytes";
            throw std::runtime_error(msg.str().c_str());
        }

        size_t offset = sizeof(T) * index;
        if (uniform.data.size() < offset + sizeof(T)) {
            uniform.data.resize(offset + sizeof(T));
        }
        std::memcpy(&uniform.data[offset], &value, sizeof(T));
    }


    template<typename T>
    void SetDynamicUniforms(uint32_t set, uint32_t binding, size_t firstIndex, const std::vector<T>& values) {
        if ((firstIndex + values.size()) > m_ObjectCount) {
            std::ostringstream msg;
            msg << "[(" << m_Name << ")->SetDynamicUniforms] Object index '" << m_ObjectCount << "' is out of bounds";
            throw std::runtime_error(msg.str().c_str());
        }

        auto it = m_UniformData.find((set << 16u) + binding);
        if (it == m_UniformData.end()) {
            std::ostringstream msg;
            msg << "[(" << m_Name << ")->SetDynamicUniforms] Uniform {" << set << ";" << binding << "} doesn't exist";
            throw std::runtime_error(msg.str().c_str());
        }

        auto &uniform = it->second;
        if (uniform.objectSize != sizeof(T)) {
            std::ostringstream msg;
            msg << "[(" << m_Name << ")->SetDynamicUniforms] Invalid size of {" << set << ";" << binding
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


    void UpdateUniforms();
};

#endif //GAME_ENGINE_MATERIAL_H
