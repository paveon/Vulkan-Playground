#ifndef VULKAN_LAYERSTACK_H
#define VULKAN_LAYERSTACK_H


#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <vulkan/vulkan_core.h>

#include "Layer.h"


class LayerStack {
private:
    using Stack = std::vector<std::unique_ptr<Layer>>;
    Stack m_Layers;
    size_t m_InsertLocation = 0;

public:
    ~LayerStack() {
        std::cout << "[Application] Deconstructing layer stack" << std::endl;
        while (!m_Layers.empty()) {
            m_Layers.back()->OnDetach();
            m_Layers.pop_back();
        }
    }

    auto PushLayer(std::unique_ptr<Layer> layer) -> Layer * {
        auto it = m_Layers.emplace(m_Layers.begin() + m_InsertLocation, std::move(layer));
        m_InsertLocation++;
        (*it)->OnAttach();
        return it->get();
    }

    auto PushOverlay(std::unique_ptr<Layer> &&overlay) -> Layer * {
        m_Layers.emplace_back(std::move(overlay));
        m_Layers.back()->OnAttach();
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

    void UpdateLayers(Timestep ts, uint32_t imageIndex) {
        for (const auto &layer : m_Layers)
            layer->OnUpdate(ts, imageIndex);
    }

    void DrawLayers(uint32_t imageIndex,
                    std::vector<VkCommandBuffer> &outBuffers,
                    const VkCommandBufferInheritanceInfo &info) {

        for (const auto &layer : m_Layers) {
            if (auto *buffer = layer->OnDraw(imageIndex, info); buffer)
                outBuffers.push_back(buffer);
        }
    }
};


#endif //VULKAN_LAYERSTACK_H
