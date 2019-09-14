#include "LayerStack.h"


std::unique_ptr<Layer> LayerStack::PopLayer(const Layer* layer) {
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


std::unique_ptr<Layer> LayerStack::PopOverlay(const Layer* overlay) {
   auto it = std::find_if(m_Layers.begin(), m_Layers.end(), [&](const auto& item) { return item.get() == overlay; });
   if (it != m_Layers.end()) {
      std::unique_ptr<Layer> tmp(std::move(*it));
      m_Layers.erase(it);
      tmp->OnDetach();
      return tmp;
   }
   return nullptr;
}
