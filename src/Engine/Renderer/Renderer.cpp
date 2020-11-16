#include <Platform/Vulkan/RendererVk.h>
#include <Engine/Model.h>
#include <Engine/ImGui/ImGuiLayer.h>
#include "Renderer.h"
#include "RenderCommand.h"


void Renderer::BeginScene(const std::shared_ptr<PerspectiveCamera> &camera) {
    Scene &scene(s_Renderer->m_Scene);
    scene.m_Meshes.clear();
    scene.m_Models.clear();
    scene.m_ModelInstances.clear();
    scene.m_ModelSet.clear();
    scene.m_MaterialSet.clear();
    scene.m_Camera = camera;
}

void Renderer::EndScene() {
    Scene &scene(s_Renderer->m_Scene);

    /// TODO: batching based on materials, etc...
    DrawPayload drawPayload{};
    DrawIndexedPayload drawIndexedPayload{};

    for (const auto* model : scene.m_ModelSet) {
        model->SetTransformUniformData(*scene.m_Camera, 2, 0);
    }

    for (auto* material : scene.m_MaterialSet) {
        material->UpdateUniforms();
    }

    for (const ModelInstance *instance : scene.m_ModelInstances) {
        Model* model = instance->GetModel();
        const Material *material = model->GetMaterial();
        if (!model->GetMaterial())
            continue;

        s_Renderer->m_CmdQueue.AddCommand(RenderCommand::BindMaterial(material));
        for (const auto &mesh : model->GetMeshes()) {
            auto materialObjIdx = mesh->GetMaterialObjectIdx();
            s_Renderer->m_CmdQueue.AddCommand(RenderCommand::BindMesh(mesh.get()));
//            s_Renderer->m_CmdQueue.AddCommand(RenderCommand::SetDynamicOffset(materialObjIdx, 0, 0));
//          for (const DynamicOffset &offset : model->GetDynamicOffsets()) {
//              s_Renderer->m_CmdQueue.AddCommand(RenderCommand::SetDynamicOffset(offset.set, offset.offset));
//          }

            if (mesh->Indices().empty()) {
                drawPayload.vertexCount = mesh->VertexCount();
                drawPayload.firstVertex = 0;
                drawPayload.firstInstance = 0;
                drawPayload.instanceCount = 1;
                s_Renderer->m_CmdQueue.AddCommand(RenderCommand::Draw(drawPayload));
            } else {
                drawIndexedPayload.indexCount = mesh->Indices().size();
                drawIndexedPayload.firstIndex = 0;
                drawIndexedPayload.vertexOffset = 0;
                drawIndexedPayload.firstInstance = 0;
                drawIndexedPayload.instanceCount = 1;
                s_Renderer->m_CmdQueue.AddCommand(RenderCommand::DrawIndexed(drawIndexedPayload));
            }
        }
    }
}

void Renderer::SubmitMesh(const Mesh *mesh) {
    s_Renderer->m_Scene.m_Meshes.push_back(mesh);
}

void Renderer::SubmitModel(const Model *model) {
    s_Renderer->m_Scene.m_Models.push_back(model);
}

void Renderer::SubmitModelInstance(const ModelInstance* instance) {
    const Model* model = instance->GetModel();
    s_Renderer->m_Scene.m_ModelSet.insert(model);
    s_Renderer->m_Scene.m_MaterialSet.insert(model->GetMaterial());
    s_Renderer->m_Scene.m_ModelInstances.push_back(instance);
}

void Renderer::SubmitCommand(const RenderCommand &cmd) {
    s_Renderer->m_CmdQueue.AddCommand(cmd);
}

void Renderer::SetImGuiLayer(Layer *layer) {
    if (auto *imgui = dynamic_cast<ImGuiLayer *>(layer)) {
        s_Renderer->m_ImGuiLayer = imgui;
    }
}

void Renderer::Init() {
    s_Renderer = std::make_unique<RendererVk>();
}


std::unique_ptr<Renderer> Renderer::s_Renderer;