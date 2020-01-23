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
#define CLAMP(lower_bound, value, upper_bound) std::max((lower_bound), std::min((upper_bound), (value)));

void InitializeGLFW();
void TerminateGLFW();


template<typename T, size_t N>
std::vector<T> ArrayToVector(const std::array<T, N>& array) {
   return std::move(std::vector<T>(array.cbegin(), array.cend()));
}


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger);


void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator);


VkDebugUtilsMessengerCreateInfoEXT createDebugMsgInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* data);

VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance instance);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

bool checkValidationLayerSupport(const std::vector<const char*>& requiredLayers);

std::vector<const char*> getRequiredVulkanExtensions(bool enableValidation);

VkInstance createVulkanInstance(const std::vector<const char*>& validationLayers);

std::vector<char> readFile(const std::string& filename);

VkImageMemoryBarrier
imageBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout);

void copyBuffer(const vk::CommandBuffer& cmdBuffer, const vk::Buffer& srcBuffer, const vk::Buffer& dstBuffer, VkDeviceSize size);

void copyBufferToImage(const vk::CommandBuffer& cmdBuffer, const vk::Buffer& buffer, const vk::Image& image);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

VkExtent2D chooseSwapExtent(VkExtent2D desiredExtent, const VkSurfaceCapabilitiesKHR& capabilities);

bool hasStencilComponent(VkFormat format);

#endif //VULKAN_UTILS_H
