#ifndef GAME_ENGINE_RENDERER_H
#define GAME_ENGINE_RENDERER_H

#include <vector>
#include <memory>
#include <Engine/Events/WindowEvents.h>
#include <unordered_map>
#include "RendererAPI.h"
#include "RenderCommand.h"


class MeshRenderer;
class RenderPass;
class Model;
class ModelInstance;
class Material;
class PerspectiveCamera;

class Scene {
public:
    std::vector<const Mesh*> m_Meshes;
    std::unordered_map<const Material*, std::vector<const MeshRenderer*>> m_MaterialBatches;
    std::shared_ptr<PerspectiveCamera> m_Camera;

    Scene() = default;
};

class Layer;
class ImGuiLayer;

struct BufferAllocation{
    void* memory;
    void* handle;
    uint64_t offset;
};

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

//    virtual void impl_StageData(void* dstBufferHandle, uint64_t* dstOffsetHandle, const void *data, uint64_t bytes) = 0;

    virtual void impl_StageMesh(Mesh* mesh) = 0;

    virtual BufferAllocation impl_AllocateUniformBuffer(uint64_t size) = 0;

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

    static void SubmitMesh(const Mesh* mesh);

    static void SubmitMeshRenderer(const MeshRenderer* mesh);

    static void SubmitModel(const Model* model);

    static void SubmitModelInstance(const ModelInstance* instance);

//    static void StageData(void* dstBufferHandle, uint64_t* dstOffsetHandle, const void *data, uint64_t bytes) {
//        s_Renderer->impl_StageData(dstBufferHandle, dstOffsetHandle, data, bytes);
//    }

    static void StageMesh(Mesh* mesh) { s_Renderer->impl_StageMesh(mesh); }

    static auto AllocateUniformBuffer(uint64_t size) -> BufferAllocation {
        return s_Renderer->impl_AllocateUniformBuffer(size);
    }

    static void FlushStagedData() { s_Renderer->impl_FlushStagedData(); }

    static auto GetSelectedAPI() -> RendererAPI::API { return RendererAPI::GetSelectedAPI(); }

    static void SubmitCommand(const RenderCommand& cmd);

    static void OnWindowResize(WindowResizeEvent& e) { s_Renderer->impl_OnWindowResize(e); }

    static auto GetRenderPass() -> const RenderPass& { return s_Renderer->impl_GetRenderPass(); }

    static auto GetImageIndex() -> size_t { return s_Renderer->impl_GetImageIndex(); }

    static void SetImGuiLayer(Layer* layer);

    static void WaitIdle() { s_Renderer->impl_WaitIdle(); }
};


#endif //GAME_ENGINE_RENDERER_H
