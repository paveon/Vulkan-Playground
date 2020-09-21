#ifndef GAME_ENGINE_RENDERER_H
#define GAME_ENGINE_RENDERER_H

#include <vector>
#include <Engine/Events/WindowEvents.h>
#include "RendererAPI.h"
#include "RenderCommand.h"
#include "ShaderProgram.h"
#include "Pipeline.h"
#include "Material.h"
#include "Mesh.h"


class PerspectiveCamera;

class Scene {
public:
    std::vector<std::shared_ptr<Mesh>> m_Meshes;
    std::shared_ptr<PerspectiveCamera> m_Camera;

    Scene() {
    }
};

class ImGuiLayer;

class Renderer {
    friend class ImGuiLayer;

protected:
    static std::unique_ptr<Renderer> s_Renderer;

    Scene m_Scene;
    RenderCommandQueue m_TransferQueue;
    RenderCommandQueue m_CmdQueue;

    ImGuiLayer* m_ImGuiLayer{};

    virtual void impl_NextFrame() = 0;

    virtual void impl_PresentFrame() = 0;

    virtual void impl_OnWindowResize(WindowResizeEvent& e) = 0;

    virtual auto impl_GetRenderPass() const -> const RenderPass& = 0;

    virtual auto impl_GetImageIndex() const -> size_t = 0;

    virtual void impl_StageData(void* dstBufferHandle, uint64_t* dstOffsetHandle, const void *data, uint64_t bytes) = 0;

    virtual void impl_FlushStagedData() = 0;

    virtual void impl_WaitIdle() const = 0;

public:
    virtual ~Renderer() = default;

    static void Init();

    static void Destroy() { s_Renderer.reset(); }

    static void NewFrame() {
        s_Renderer->impl_NextFrame();
    }

    static void PresentFrame() {
        s_Renderer->impl_PresentFrame();
        s_Renderer->m_CmdQueue.Clear();
    }

    static void BeginScene(const std::shared_ptr<PerspectiveCamera> &camera);

    static void EndScene();

    static void SubmitMesh(const std::shared_ptr<Mesh> &mesh);

    static void StageData(void* dstBufferHandle, uint64_t* dstOffsetHandle, const void *data, uint64_t bytes) {
        s_Renderer->impl_StageData(dstBufferHandle, dstOffsetHandle, data, bytes);
    }

    static void FlushStagedData() { s_Renderer->impl_FlushStagedData(); }

    static auto GetSelectedAPI() -> RendererAPI::API { return RendererAPI::GetSelectedAPI(); }

    static void SubmitCommand(const RenderCommand& cmd);

    static void OnWindowResize(WindowResizeEvent& e) { s_Renderer->impl_OnWindowResize(e); }

    static auto GetRenderPass() -> const RenderPass& { return s_Renderer->impl_GetRenderPass(); }

    static auto GetImageIndex() -> size_t { return s_Renderer->impl_GetImageIndex(); }

    static void SetImGuiLayer(ImGuiLayer* layer) { s_Renderer->m_ImGuiLayer = layer; }

    static void WaitIdle() { s_Renderer->impl_WaitIdle(); }
};


#endif //GAME_ENGINE_RENDERER_H
