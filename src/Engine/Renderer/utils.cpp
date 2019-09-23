#include "utils.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <array>
#include <limits>
#include <cstring>

using namespace vk;

static bool s_GLFWInitialized = false;


void GLFWErrorCallback(int error, const char* description) {
   std::cerr << "[GLFW][Error " << error << "] " << description << std::endl;
}


void InitializeGLFW() {
   if (!s_GLFWInitialized) {
      if (!glfwInit()) throw std::runtime_error("GLFW initialization failed");
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwSetErrorCallback(GLFWErrorCallback);
      s_GLFWInitialized = true;
   }
}


void TerminateGLFW() {
   if (s_GLFWInitialized) {
      glfwTerminate();
      s_GLFWInitialized = false;
   }
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {

   auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
   if (func != nullptr) {
      return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
   } else {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
   }
}


void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
   auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
   if (func != nullptr) {
      func(instance, debugMessenger, pAllocator);
   }
}


VkDebugUtilsMessengerCreateInfoEXT createDebugMsgInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* data) {
   VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
   createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   createInfo.pfnUserCallback = callback;
   createInfo.pUserData = data;

   return createInfo;
}



VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
   (void) pUserData;

   std::cout << "[Vulkan]";

   if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
      std::cout << "[General]";
   } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
      std::cout << "[Validation]";
   } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
      std::cout << "[Performance]";
   }

   if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
      std::cout << "[ERROR]";
   } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
      std::cout << "[WARNING]";
   } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
      std::cout << "[INFO]";
   }
   std::cout << " " << pCallbackData->pMessage << std::endl;

   return VK_FALSE;
}


bool checkValidationLayerSupport(const std::vector<const char*>& requiredLayers) {
   uint32_t layerCount;
   vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

   std::vector<VkLayerProperties> availableLayers(layerCount);
   vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

   for (const char* layerName : requiredLayers) {
      bool layerFound = false;

      for (const auto& layerProperties : availableLayers) {
         if (std::strcmp(layerName, layerProperties.layerName) == 0) {
            layerFound = true;
            break;
         }
      }

      if (!layerFound) { return false; }
   }

   return true;
}


std::vector<const char*> getRequiredVulkanExtensions(bool enableValidation) {
   uint32_t extensionCount;
   const char** glfwExtensions;
   InitializeGLFW();

   glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
   std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionCount);
   if (enableValidation)
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

   return extensions;
}


VkInstance createVulkanInstance(const std::vector<const char*>& validationLayers) {
   const bool enableValidation = !validationLayers.empty();
   if (enableValidation && !checkValidationLayerSupport(validationLayers)) {
      throw std::runtime_error("[Vulkan] validation layers requested, but not available!");
   }

   VkApplicationInfo appInfo = {};
   appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName = "Renderer";
   appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.pEngineName = "No Engine";
   appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.apiVersion = VK_API_VERSION_1_1;

   VkInstanceCreateInfo createInfo = {};
   createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   createInfo.pApplicationInfo = &appInfo;
   auto extensions = getRequiredVulkanExtensions(enableValidation);
   createInfo.enabledExtensionCount = extensions.size();
   createInfo.ppEnabledExtensionNames = extensions.data();

   VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
   if (enableValidation) {
      createInfo.enabledLayerCount = validationLayers.size();
      createInfo.ppEnabledLayerNames = validationLayers.data();
      debugCreateInfo = createDebugMsgInfo(debugCallback, nullptr);
      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
   }

   VkInstance instance;
   if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
      throw std::runtime_error("Vulkan instance could not be created!");

   return instance;
}


std::vector<char> readFile(const std::string& filename) {
   std::ifstream file(filename, std::ios::ate | std::ios::binary);
   if (!file.is_open()) {
      throw std::runtime_error("failed to open file!");
   }

   size_t fileSize = (size_t) file.tellg();
   std::vector<char> buffer(fileSize);

   file.seekg(0);
   file.read(buffer.data(), fileSize);
   file.close();
   return buffer;
}


VkImageMemoryBarrier
imageBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout) {
   VkImageMemoryBarrier result = {};
   result.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   result.srcAccessMask = srcAccessMask;
   result.dstAccessMask = dstAccessMask;
   result.oldLayout = oldLayout;
   result.newLayout = newLayout;
   result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   result.image = image;
   result.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
   result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

   return result;
}


void copyBuffer(const CommandBuffer& cmdBuffer, const Buffer& srcBuffer, const Buffer& dstBuffer, VkDeviceSize size) {
   VkBufferCopy copyRegion = {};
   copyRegion.srcOffset = 0; // Optional
   copyRegion.dstOffset = 0; // Optional
   copyRegion.size = size;
   vkCmdCopyBuffer(cmdBuffer.data(), srcBuffer.data(), dstBuffer.data(), 1, &copyRegion);
}


void copyBufferToImage(const CommandBuffer& cmdBuffer, const Buffer& buffer, const Image& image) {
   VkBufferImageCopy region = {};
   region.bufferOffset = 0;
   region.bufferRowLength = 0;
   region.bufferImageHeight = 0;

   region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   region.imageSubresource.mipLevel = 0;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = 1;

   region.imageOffset = {0, 0, 0};
   region.imageExtent = {image.Extent.width, image.Extent.height, 1};

   vkCmdCopyBufferToImage(cmdBuffer.data(), buffer.data(), image.data(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}


VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
   for (const auto& availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
         return availableFormat;
      }
   }

   return availableFormats[0];
}


VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>&) {
   VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

//      for (const auto& availablePresentMode : availablePresentModes) {
//         if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
//            return availablePresentMode;
//         }
//         else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
//            bestMode = availablePresentMode;
//         }
//      }

   return bestMode;
}


VkExtent2D chooseSwapExtent(VkExtent2D desiredExtent, const VkSurfaceCapabilitiesKHR& capabilities) {
   if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
   } else {
      desiredExtent.width = CLAMP(capabilities.minImageExtent.width, desiredExtent.width, capabilities.maxImageExtent.width);
      desiredExtent.height = CLAMP(capabilities.minImageExtent.height, desiredExtent.height, capabilities.maxImageExtent.height);
      return {desiredExtent.width, desiredExtent.height};
   }
}

bool hasStencilComponent(VkFormat format) { return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; }