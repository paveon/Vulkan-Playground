#ifndef VULKAN_LAYERSTACK_H
#define VULKAN_LAYERSTACK_H


#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <vulkan/vulkan_core.h>

#include "Layer.h"
#include "Core.h"

class Application;

class LayerStack {
    friend class Layer;

private:
    using Stack = std::vector<std::unique_ptr<Layer>>;
    Application* m_Parent;
    Stack m_Layers;
    size_t m_InsertLocation = 0;

public:
    explicit LayerStack(Application* parent) : m_Parent(parent) {}

    ~LayerStack();

    auto PushLayer(std::unique_ptr<Layer> layer) -> Layer * {
        auto it = m_Layers.emplace(m_Layers.begin() + m_InsertLocation, std::move(layer));
        m_InsertLocation++;
        (*it)->OnAttach(this);
        return it->get();
    }

    auto PushOverlay(std::unique_ptr<Layer> overlay) -> Layer * {
        m_Layers.emplace_back(std::move(overlay));
        m_Layers.back()->OnAttach(this);
        return m_Layers.back().get();
    }

    auto PopLayer(const Layer *layer) -> std::unique_ptr<Layer>;

    auto PopOverlay(const Layer *overlay) -> std::unique_ptr<Layer>;

    auto begin() -> Stack::iterator { return m_Layers.begin(); }

    auto end() -> Stack::iterator { return m_Layers.end(); }

    template<typename T>
    using LayerCallbackFn = bool (Layer::*)(T &);

    template<typename T>
    void PropagateEvent(T &event, LayerCallbackFn<T> callbackPtr) {
        for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); it++) {
            Layer *layer = it->get();
            (layer->*callbackPtr)(event);
            if (event.Handled)
                break;
        }
    }

    void UpdateLayers(Timestep ts) {
        for (const auto &layer : m_Layers)
            layer->OnUpdate(ts);
    }

    void DrawLayers() {
        for (const auto &layer : m_Layers) {
            layer->OnDraw();
            layer->OnImGuiDraw();
        }
    }
};


#endif //VULKAN_LAYERSTACK_H
