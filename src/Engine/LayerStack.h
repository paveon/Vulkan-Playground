#ifndef VULKAN_LAYERSTACK_H
#define VULKAN_LAYERSTACK_H


#include <vector>
#include <memory>
#include <algorithm>
#include "Layer.h"

class LayerStack {
private:
    using Stack = std::vector<std::unique_ptr<Layer>>;
    Stack m_Layers;
    size_t m_InsertLocation = 0;

public:
    Layer* PushLayer(std::unique_ptr<Layer> layer) {
       auto it = m_Layers.emplace(m_Layers.begin() + m_InsertLocation, std::move(layer));
       m_InsertLocation++;
       (*it)->OnAttach();
       return it->get();
    }

    Layer* PushOverlay(std::unique_ptr<Layer>&& overlay) {
       m_Layers.emplace_back(std::move(overlay));
       m_Layers.back()->OnAttach();
       return m_Layers.back().get();
    }

    std::unique_ptr<Layer> PopLayer(const Layer* layer);

    std::unique_ptr<Layer> PopOverlay(const Layer* overlay);

    Stack::iterator begin() { return m_Layers.begin(); }

    Stack::iterator end() { return m_Layers.end(); }

    template<typename T>
    using LayerCallbackFn = bool(Layer::*)(T&);

    template<typename T>
    void PropagateEvent(T& event, LayerCallbackFn<T> callbackPtr) {
       for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); it++) {
          Layer* layer = it->get();
          (layer->*callbackPtr)(event);
          if (event.Handled)
             break;
       }
    }

   void UpdateLayers() {
       for (const auto& layer : m_Layers)
          layer->OnUpdate();
    }
};


#endif //VULKAN_LAYERSTACK_H
