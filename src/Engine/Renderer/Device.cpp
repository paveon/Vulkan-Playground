#include "Device.h"

#include <iostream>
#include <set>

using namespace vk;


Device::Device(const vk::Instance &instance, const vk::Surface &surface) : m_Instance(&instance), m_Surface(&surface) {
    m_PhysicalDevice = pickPhysicalDevice();
    m_LogicalDevice = vk::LogicalDevice(m_PhysicalDevice, surface.data());
    m_GfxCmdPool = createCommandPool(GfxQueueIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_TransferCmdPool = createCommandPool(TransferQueueIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
}


auto Device::pickPhysicalDevice() -> VkPhysicalDevice {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance->data(), &deviceCount, nullptr);
    if (deviceCount == 0) { throw std::runtime_error("No GPU with Vulkan support!"); }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance->data(), &deviceCount, devices.data());

    VkPhysicalDevice fallback = nullptr;
    VkPhysicalDeviceFeatures supportedFeatures;
    for (const auto &device : devices) {
        vkGetPhysicalDeviceProperties(device, &m_Properties);
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        if (supportedFeatures.samplerAnisotropy) {
            if (m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                std::cout << "[Vulkan] Picking discrete GPU: " << m_Properties.deviceName << std::endl;
                VkSampleCountFlags counts = std::min(m_Properties.limits.framebufferColorSampleCounts,
                                                     m_Properties.limits.framebufferDepthSampleCounts);

                if (counts & VK_SAMPLE_COUNT_64_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_64_BIT; }
                else if (counts & VK_SAMPLE_COUNT_32_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_32_BIT; }
                else if (counts & VK_SAMPLE_COUNT_16_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_16_BIT; }
                else if (counts & VK_SAMPLE_COUNT_8_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_8_BIT; }
                else if (counts & VK_SAMPLE_COUNT_4_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_4_BIT; }
                else if (counts & VK_SAMPLE_COUNT_2_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_2_BIT; }
                return device;
            } else if (!fallback) {
                fallback = device;
            }
        }
    }

    if (!fallback) throw std::runtime_error("No suitable GPU!");

    vkGetPhysicalDeviceProperties(fallback, &m_Properties);
    std::cout << "[Vulkan] Picking fallback GPU: " << m_Properties.deviceName << std::endl;
    return fallback;
}


auto Device::findMemoryType(
        const VkMemoryRequirements &requirements,
        VkMemoryPropertyFlags properties
) const -> uint32_t {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((requirements.memoryTypeBits & (1UL << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}


auto Device::createSwapChain(const std::set<uint32_t> &queueIndices, VkExtent2D extent) -> Swapchain * {
    VkSurfaceCapabilitiesKHR capabilities = getSurfaceCapabilities();
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(m_LogicalDevice.getSurfaceFormats());
    VkPresentModeKHR presentMode = chooseSwapPresentMode(m_LogicalDevice.getSurfacePresentModes());
    m_Swapchain = std::make_unique<Swapchain>(m_LogicalDevice.data(), queueIndices, extent, m_Surface->data(),
                                              capabilities,
                                              surfaceFormat, presentMode);
    return m_Swapchain.get();
}


auto Device::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features) const -> VkFormat {
    for (VkFormat format : candidates) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &properties);

        switch (tiling) {
            case VK_IMAGE_TILING_LINEAR:
                if ((properties.linearTilingFeatures & features) == features) return format;
                break;

            case VK_IMAGE_TILING_OPTIMAL:
                if ((properties.optimalTilingFeatures & features) == features) return format;
                break;

            default:
                continue;
        }
    }

    throw std::runtime_error("failed to find desirable supported format!");
}

void Device::Release() {
    std::cout << "[Device] Destroying rendering device" << std::endl;
}
