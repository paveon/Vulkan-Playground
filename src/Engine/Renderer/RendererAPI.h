#ifndef GAME_ENGINE_RENDERER_API_H
#define GAME_ENGINE_RENDERER_API_H


class RendererAPI {
public:
    enum class API {
        VULKAN = 0
    };

private:
    static API s_SelectedAPI;

public:
    virtual ~RendererAPI() = default;

    static void SelectAPI(API) {};

    static auto GetSelectedAPI() -> API { return s_SelectedAPI; }
};


#endif //GAME_ENGINE_RENDERER_API_H
