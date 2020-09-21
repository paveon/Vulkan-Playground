#include "LayerStack.h"
#include "Application.h"


auto LayerStack::PopLayer(const Layer* layer) -> std::unique_ptr<Layer> {
   auto it = std::find_if(m_Layers.begin(), m_Layers.end(), [&](const auto& item) { return item.get() == layer; });
   if (it != m_Layers.end()) {
      std::unique_ptr<Layer> tmp(std::move(*it));
      m_Layers.erase(it);
      m_InsertLocation--;
      tmp->OnDetach();
      return tmp;
   }
   return nullptr;
}


auto LayerStack::PopOverlay(const Layer* overlay) -> std::unique_ptr<Layer> {
   auto it = std::find_if(m_Layers.begin(), m_Layers.end(), [&](const auto& item) { return item.get() == overlay; });
   if (it != m_Layers.end()) {
      std::unique_ptr<Layer> tmp(std::move(*it));
      m_Layers.erase(it);
      tmp->OnDetach();
      return tmp;
   }
   return nullptr;
}

LayerStack::~LayerStack() {
    std::cout << currentTime() << "[" << m_Parent->GetName() << "][LayerStack] Deconstructing" << std::endl;
    while (!m_Layers.empty()) {
        m_Layers.back()->OnDetach();
        m_Layers.pop_back();
    }
}
