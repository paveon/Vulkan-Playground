#include "ShaderProgramVk.h"

#include <Engine/Application.h>
#include "GraphicsContextVk.h"


ShaderProgramVk::ShaderProgramVk(const std::vector<std::pair<const char *, ShaderType>> &shaders,
                                 const std::vector<uint32_t> &dynamics) {
    auto &device = static_cast<GfxContextVk &>(Application::GetGraphicsContext()).GetDevice();

    for (const auto&[filepath, shaderType] : shaders) {
        auto *module = device.createShaderModule(filepath);

        /// Merge descriptor set layouts from all stages
        for (const auto&[setIdx, setLayout] : module->GetSetLayouts()) {
            auto &layout = m_DescriptorSetLayouts[setIdx];
            for (const auto&[bindingIdx, binding] : setLayout) {
                if (layout.find(bindingIdx) == layout.end()) {
                    layout[bindingIdx] = binding;

                    if (binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                        uint32_t key = (setIdx << 16u) + bindingIdx;
                        bool dynamic = std::find(dynamics.begin(), dynamics.end(), key) != dynamics.end();
                        if (dynamic) layout[bindingIdx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

                        m_UniformObjects.emplace_back(ShaderProgram::UniformObject{
                                binding.name,
                                (setIdx << 16u) + bindingIdx,
                                binding.size,
                                dynamic
                        });
                    }
                }
            }
        }
        m_ShaderModules[shaderType] = module;
    }
    m_VertexBindings = &m_ShaderModules[ShaderType::VERTEX_SHADER]->GetInputBindings();
    for (const auto &attribute : m_VertexBindings->at(0).vertexLayout) {
        m_VertexLayout.push_back(attribute.size);
    }

    /// TODO: more robust solution, this is naive and might break
    /// Check if input of each subsequent shader stage matches output of the previous one
    auto endIt = std::prev(m_ShaderModules.end(), 1);
    for (auto it = m_ShaderModules.begin(); it != endIt;) {
        const auto &output = it->second->GetOutputBindings();
        const auto &nextStage = std::next(it, 1);
        const auto &nextInput = nextStage->second->GetInputBindings();
        const auto &outputLayout = output.front().vertexLayout;
        const auto &nextInputLayout = nextInput.front().vertexLayout;

        if (outputLayout.size() != nextInputLayout.size()) {
            std::ostringstream msg;
            msg << "[ShaderProgramVk] Stage '" << ShaderTypeToString(it->first) << "' with '" << outputLayout.size()
                << "' output vertex attributes mismatches stage '" << ShaderTypeToString(nextStage->first)
                << "' with '" << nextInputLayout.size() << "' input vertex attributes";
            throw std::runtime_error(msg.str().c_str());
        }

        for (size_t i = 0; i < outputLayout.size(); i++) {
            if (outputLayout[i] != nextInputLayout[i]) {
                std::ostringstream msg;
                msg << "[ShaderProgramVk] Output vertex attribute of stage '" << ShaderTypeToString(it->first)
                    << "' with location '" << i << "' mismatches input attribute of next stage '"
                    << ShaderTypeToString(nextStage->first) << "'";
                throw std::runtime_error(msg.str().c_str());
            }
        }
        it = nextStage;
    }
}

auto ShaderProgramVk::createInfo() const -> std::vector<VkPipelineShaderStageCreateInfo> {
    std::vector<VkPipelineShaderStageCreateInfo> stagesCreateInfo;
    for (const auto &it: m_ShaderModules) {
        VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
        switch (it.first) {
            case ShaderType::VERTEX_SHADER:
                shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case ShaderType::FRAGMENT_SHADER:
                shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case ShaderType::GEOMETRY_SHADER:
                shaderStage = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case ShaderType::TESSELATION_SHADER:
                shaderStage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
            case ShaderType::COMPUTE_SHADER:
                shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
        }

        stagesCreateInfo.emplace_back(VkPipelineShaderStageCreateInfo{
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                nullptr,
                0,
                shaderStage,
                it.second->data(),
                "main",
                nullptr
        });
    }

    return stagesCreateInfo;
}
