#include <Platform/Vulkan/RendererVk.h>
#include <Engine/Model.h>
#include <Engine/ImGui/ImGuiLayer.h>
#include "Renderer.h"
#include "RenderCommand.h"


void Renderer::BeginScene(const std::shared_ptr<PerspectiveCamera> &camera) {
    Scene &scene(s_Renderer->m_Scene);
    scene.m_Meshes.clear();
    scene.m_MaterialBatches.clear();
//    scene.m_Models.clear();
//    scene.m_ModelInstances.clear();
//    scene.m_ModelSet.clear();
//    scene.m_MaterialSet.clear();
    scene.m_Camera = camera;
}

void Renderer::EndScene() {
    Scene &scene(s_Renderer->m_Scene);

    /// TODO: batching based on materials, etc...
    DrawPayload drawPayload{};
    DrawIndexedPayload drawIndexedPayload{};

//    for (const auto* model : scene.m_ModelSet) {
//        model->SetTransformUniformData(*scene.m_Camera, 2, 0);
//    }
//
//    for (auto* material : scene.m_MaterialSet) {
//        material->UpdateUniforms();
//    }
    if (scene.m_SkyboxMesh) {
        const Material* skyboxMaterial = scene.m_SkyboxMesh->GetMaterial();
        s_Renderer->m_CmdQueue.AddCommand(RenderCommand::BindMaterial(skyboxMaterial));
        s_Renderer->m_CmdQueue.AddCommand(RenderCommand::BindMeshInstance(scene.m_SkyboxMesh));

        const auto* mesh = scene.m_SkyboxMesh->GetMesh();
        drawPayload.vertexCount = mesh->VertexCount();
        drawPayload.firstVertex = 0;
        drawPayload.firstInstance = 0;
        drawPayload.instanceCount = 1;
        s_Renderer->m_CmdQueue.AddCommand(RenderCommand::Draw(drawPayload));
    }

    for (const auto&[material, batch] : scene.m_MaterialBatches) {
        s_Renderer->m_CmdQueue.AddCommand(RenderCommand::BindMaterial(material));

        for (const MeshRenderer *meshInstance : batch) {
            const auto *mesh = meshInstance->GetMesh();
            uint32_t entityID = meshInstance->ParentEntityID();
            //            auto materialObjIdx = mesh->GetMaterialObjectIdx();
//            auto meshID = meshInstance->GetMaterialInstance().InstanceID();
            s_Renderer->m_CmdQueue.AddCommand(RenderCommand::BindMeshInstance(meshInstance));
            s_Renderer->m_CmdQueue.AddCommand(RenderCommand::SetDynamicOffset(0, 0, entityID));
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


void Renderer::SubmitMeshRenderer(const MeshRenderer *meshInstance) {
    auto &batches = s_Renderer->m_Scene.m_MaterialBatches;
    const Material *meshMaterial = meshInstance->GetMaterial();
    auto it = batches.find(meshMaterial);
    if (it == batches.end()) {
        it = batches.insert({meshMaterial, {}}).first;
    }
    it->second.push_back(meshInstance);
}


void Renderer::SubmitSkybox(const MeshRenderer *meshInstance) {
    s_Renderer->m_Scene.m_SkyboxMesh = meshInstance;
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