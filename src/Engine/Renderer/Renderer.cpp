#include <Platform/Vulkan/RendererVk.h>
#include "Renderer.h"
#include "RenderCommand.h"



void Renderer::BeginScene(const std::shared_ptr<PerspectiveCamera> &camera) {
    Scene &scene(s_Renderer->m_Scene);
    scene.m_Meshes.clear();
    scene.m_Camera = camera;
}

void Renderer::EndScene() {
    Scene &scene(s_Renderer->m_Scene);
    for (const auto &mesh : scene.m_Meshes) {
        s_Renderer->m_CmdQueue.AddCommand(RenderCommand::BindVertexBuffer(&mesh->VertexBuffer(), 0));
        s_Renderer->m_CmdQueue.AddCommand(RenderCommand::BindIndexBuffer(&mesh->IndexBuffer(), 0));
        s_Renderer->m_CmdQueue.AddCommand(RenderCommand::BindMaterial(&mesh->GetMaterial()));
        s_Renderer->m_CmdQueue.AddCommand(RenderCommand::DrawIndexed(mesh->IndexBuffer().IndexCount(), 0, 0));
    }
//    RenderCommand::DrawIndexed(mesh.VertexBuffer(), mesh.IndexBuffer());
}

void Renderer::SubmitMesh(const std::shared_ptr<Mesh> &mesh) {
    s_Renderer->m_Scene.m_Meshes.push_back(mesh);
}

void Renderer::SubmitCommand(const RenderCommand &cmd) {
    s_Renderer->m_CmdQueue.AddCommand(cmd);
}

void Renderer::Init() {
    s_Renderer = std::make_unique<RendererVk>();
}



std::unique_ptr<Renderer> Renderer::s_Renderer;