#include "utils.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <array>
#include <limits>
#include <cstring>
#include <Engine/Core.h>
#include <sstream>

static bool s_GLFWInitialized = false;


void GLFWErrorCallback(int error, const char *description) {
   std::cerr << "[GLFW][Error " << error << "] " << description << std::endl;
}


void InitializeGLFW(const std::vector<std::pair<int, int>> &windowHints) {
   if (!s_GLFWInitialized) {
      if (!glfwInit()) throw std::runtime_error("GLFW initialization failed");
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      for (const auto& hint : windowHints)
         glfwWindowHint(hint.first, hint.second);

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

auto CreateDebugUtilsMessengerEXT(VkInstance instance,
                                  const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                  const VkAllocationCallbacks *pAllocator,
                                  VkDebugUtilsMessengerEXT *pDebugMessenger) -> VkResult {

   auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
   if (func != nullptr) {
      return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
   } else {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
   }
}


void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
   auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                           "vkDestroyDebugUtilsMessengerEXT");
   if (func != nullptr) {
      func(instance, debugMessenger, pAllocator);
   }
}


auto createDebugMsgInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback,
                        void *data) -> VkDebugUtilsMessengerCreateInfoEXT {
   VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
   createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   createInfo.messageSeverity =
           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   createInfo.pfnUserCallback = callback;
   createInfo.pUserData = data;

   return createInfo;
}


VKAPI_ATTR auto VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) -> VkBool32 {
   (void) pUserData;

   std::ostringstream ss;
   ss << currentTime() << "[Vulkan]";

   if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
      ss << "[General]";
   } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
      ss << "[Validation]";
   } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
      ss << "[Performance]";
   }

   if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
      ss << "[ERROR]";
   } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
      ss << "[WARNING]";
   } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
      ss << "[INFO]";
   }
   ss << " " << pCallbackData->pMessage;
   std::cerr << ss.str() << std::endl;

   return VK_FALSE;
}


auto checkLayerSupport(const std::vector<const char *> &requiredLayers,
                       const std::vector<const char *> &optionalLayers)
-> std::vector<const char *> {

   std::vector<const char *> layers;
   layers.reserve(requiredLayers.size() + optionalLayers.size());

   uint32_t layerCount = 0;
   vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

   std::vector<VkLayerProperties> availableLayers(layerCount);
   vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

   for (const char *layerName: requiredLayers) {
//        bool layerFound = false;
//
//        for (const auto &layerProperties : availableLayers) {
//            if (std::strcmp(layerName, layerProperties.layerName) == 0) {
//                layerFound = true;
//                break;
//            }
//        }
      auto it = std::find_if(availableLayers.begin(), availableLayers.end(), [&](const auto &props) {
         return std::strcmp(layerName, props.layerName) == 0;
      });

      if (it == availableLayers.end()) {
         std::ostringstream errMsg;
         errMsg << "[Vulkan] Layer '" << layerName << "' requested, but not available!";
         throw std::runtime_error(errMsg.str());
      }
      layers.push_back(layerName);
   }

   for (const char *layerName: optionalLayers) {
      auto it = std::find_if(availableLayers.begin(), availableLayers.end(), [&](const auto &props) {
         return std::strcmp(layerName, props.layerName) == 0;
      });
      if (it != availableLayers.end())
         layers.push_back(layerName);
   }

   return layers;
}


auto getRequiredVulkanExtensions(bool enableValidation) -> std::vector<const char *> {
   uint32_t extensionCount = 0;
   const char **glfwExtensions = nullptr;
   InitializeGLFW({});

   glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
   std::vector<const char *> extensions(glfwExtensions, glfwExtensions + extensionCount);
   if (enableValidation)
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

   return extensions;
}


auto readFile(const std::string &filename) -> std::vector<uint32_t> {
   FILE *file = fopen(filename.c_str(), "rb");
   if (!file) {
      fprintf(stderr, "Failed to open SPIR-V file: %s\n", filename.c_str());
      return {};
   }

   fseek(file, 0, SEEK_END);
   size_t len = std::ftell(file) / sizeof(uint32_t);
   rewind(file);

   std::vector<uint32_t> spirv(len);
   if (fread(spirv.data(), sizeof(uint32_t), len, file) != size_t(len))
      spirv.clear();

   fclose(file);
   return spirv;
}


auto imageBarrier(VkImage image,
                  VkAccessFlags srcAccessMask,
                  VkAccessFlags dstAccessMask,
                  VkImageLayout oldLayout,
                  VkImageLayout newLayout) -> VkImageMemoryBarrier {
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


void copyBuffer(VkCommandBuffer cmdBuffer,
                const vk::Buffer &srcBuffer,
                const vk::Buffer &dstBuffer,
                VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0) {
   VkBufferCopy copyRegion = {};
   copyRegion.srcOffset = srcOffset;
   copyRegion.dstOffset = dstOffset;
   copyRegion.size = size;
   vkCmdCopyBuffer(cmdBuffer, srcBuffer.data(), dstBuffer.data(), 1, &copyRegion);
}


void copyBufferToImage(const vk::CommandBuffer &cmdBuffer, const vk::Buffer &buffer, const vk::Image &image) {
   VkBufferImageCopy region = {};
   region.bufferOffset = 0;
   region.bufferRowLength = 0;
   region.bufferImageHeight = 0;

   region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   region.imageSubresource.mipLevel = 0;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = image.Info().arrayLayers;

   region.imageOffset = {0, 0, 0};
   region.imageExtent = {image.Extent().width, image.Extent().height, 1};

   vkCmdCopyBufferToImage(cmdBuffer.data(), buffer.data(), image.data(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                          &region);
}


auto chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) -> VkSurfaceFormatKHR {
   for (const auto &availableFormat: availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
         return availableFormat;
      }

//        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
//            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
//            return availableFormat;
//        }
   }

   return availableFormats[0];
}


auto chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &) -> VkPresentModeKHR {
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


auto chooseSwapExtent(VkExtent2D desiredExtent, const VkSurfaceCapabilitiesKHR &capabilities) -> VkExtent2D {
   if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
   } else {
      desiredExtent.width = math::CLAMP(capabilities.minImageExtent.width, desiredExtent.width,
                                        capabilities.maxImageExtent.width);
      desiredExtent.height = math::CLAMP(capabilities.minImageExtent.height, desiredExtent.height,
                                         capabilities.maxImageExtent.height);
      return {desiredExtent.width, desiredExtent.height};
   }
}

auto hasStencilComponent(VkFormat format) -> bool {
   return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

auto findMemoryType(VkPhysicalDevice physDevice, uint32_t typeBits, VkMemoryPropertyFlags flags) -> uint32_t {
   VkPhysicalDeviceMemoryProperties memProperties;
   vkGetPhysicalDeviceMemoryProperties(physDevice, &memProperties);

   for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      auto supportedFlags = memProperties.memoryTypes[i].propertyFlags;
      if ((typeBits & (1UL << i)) && (supportedFlags & flags) == flags) {
         return i;
      }
   }

   throw std::runtime_error("failed to find suitable memory type!");
}