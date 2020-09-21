#ifndef GAME_ENGINE_MATERIAL_H
#define GAME_ENGINE_MATERIAL_H

#include <memory>
#include "Pipeline.h"


class Material {
private:
    std::unique_ptr<Pipeline> m_Pipeline;

public:
    Material(const std::unique_ptr<ShaderProgram>& vs, const std::unique_ptr<ShaderProgram>& fs);

    auto GetPipeline() const -> const Pipeline& { return *m_Pipeline; }

    void BindUniformBuffer(const std::unique_ptr<UniformBuffer>& ub, size_t binding) {
        m_Pipeline->BindUniformBuffer(*ub, binding);
    }

    void BindTexture(const std::unique_ptr<Texture2D>& texture, size_t binding) {
        m_Pipeline->BindTexture(*texture, binding);
    }
};

#endif //GAME_ENGINE_MATERIAL_H
