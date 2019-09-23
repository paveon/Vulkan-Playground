#ifndef VULKAN_GRAPHICSCONTEXTVK_H
#define VULKAN_GRAPHICSCONTEXTVK_H

#include "Engine/Renderer/vulkan_wrappers.h"
#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/GraphicsContext.h"

struct GLFWwindow;

class GfxContextVk : public GfxContext {
private:
    GLFWwindow* m_WindowHandle = nullptr;
    vk::Instance m_Instance;
    vk::Surface m_Surface;
    Device m_Device;

public:
    explicit GfxContextVk(GLFWwindow* windowHandle);

    void Init() override;

    const vk::Instance& Instance() const { return m_Instance; }

    const vk::Surface& Surface() const { return m_Surface; }

    const Device& Device() const { return m_Device; }
};

#endif //VULKAN_GRAPHICSCONTEXTVK_H
