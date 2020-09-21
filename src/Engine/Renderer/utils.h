#ifndef VULKAN_UTILS_H
#define VULKAN_UTILS_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <chrono>
#include <Engine/Window.h>
#include "vulkan_wrappers.h"


#define TIME_NOW std::chrono::steady_clock::now()

void InitializeGLFW();

void TerminateGLFW();


template<typename T, size_t N>
auto ArrayToVector(const std::array<T, N> &array) -> std::vector<T> {
    return std::move(std::vector<T>(array.cbegin(), array.cend()));
}


auto CreateDebugUtilsMessengerEXT(VkInstance instance,
                                  const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                  const VkAllocationCallbacks *pAllocator,
                                  VkDebugUtilsMessengerEXT *pDebugMessenger) -> VkResult;


void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator);


auto createDebugMsgInfo(
        PFN_vkDebugUtilsMessengerCallbackEXT callback,
        void *data) -> VkDebugUtilsMessengerCreateInfoEXT;

auto setupDebugMessenger(VkInstance instance) -> VkDebugUtilsMessengerEXT;

VKAPI_ATTR auto VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) -> VkBool32;

auto checkValidationLayerSupport(const std::vector<const char *> &requiredLayers) -> bool;

auto getRequiredVulkanExtensions(bool enableValidation) -> std::vector<const char *>;

auto createVulkanInstance(const std::vector<const char *> &validationLayers) -> VkInstance;

auto readFile(const std::string &filename) -> std::vector<uint32_t>;

auto imageBarrier(VkImage image,
                  VkAccessFlags srcAccessMask,
                  VkAccessFlags dstAccessMask,
                  VkImageLayout oldLayout,
                  VkImageLayout newLayout) -> VkImageMemoryBarrier;

void copyBuffer(const vk::CommandBuffer &cmdBuffer,
                const vk::Buffer &srcBuffer,
                const vk::Buffer &dstBuffer,
                VkDeviceSize size);

void copyBufferToImage(const vk::CommandBuffer &cmdBuffer, const vk::Buffer &buffer, const vk::Image &image);

auto chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) -> VkSurfaceFormatKHR;

auto chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) -> VkPresentModeKHR;

auto chooseSwapExtent(VkExtent2D desiredExtent, const VkSurfaceCapabilitiesKHR &capabilities) -> VkExtent2D;

auto hasStencilComponent(VkFormat format) -> bool;

auto findMemoryType(VkPhysicalDevice physDevice, uint32_t typeBits, VkMemoryPropertyFlags flags) -> uint32_t;

#endif //VULKAN_UTILS_H
