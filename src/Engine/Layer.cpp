#include "Layer.h"
#include "LayerStack.h"
#include "Application.h"

void Layer::OnAttach(const LayerStack *stack) {
    m_Parent = stack;
    const std::string& appName = m_Parent->m_Parent->GetName();
    std::cout << currentTime() << "[" << appName << "][LayerStack] " << m_DebugName << "::Attached" << std::endl;
}

void Layer::OnDetach() {
    const std::string& appName = m_Parent->m_Parent->GetName();
    std::cout << currentTime() << "[" << appName << "][LayerStack] " << m_DebugName << "::Detached" << std::endl;
    m_Parent = nullptr;
}
