#define GLFW_INCLUDE_VULKAN
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include "vulkan_wrappers.h"
#include "utils.h"
#include <fstream>

#include <GLFW/glfw3.h>
#include <unordered_map>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <iostream>

namespace std {
    template<>
    struct hash<math::vec2> {
        size_t operator()(const math::vec2& v) const {
           size_t res = 0;
           hash_combine(res, v.x);
           hash_combine(res, v.y);
           return res;
        }
    };

    template<>
    struct hash<math::vec3> {
        size_t operator()(const math::vec3& v) const {
           size_t res = 0;
           hash_combine(res, v.x);
           hash_combine(res, v.y);
           hash_combine(res, v.z);
           return res;
        }
    };

    template<>
    struct hash<math::vec4> {
        size_t operator()(const math::vec4& v) const {
           size_t res = 0;
           hash_combine(res, v.x);
           hash_combine(res, v.y);
           hash_combine(res, v.z);
           hash_combine(res, v.w);
           return res;
        }
    };

    template<>
    struct hash<Vertex> {
        size_t operator()(const Vertex& vertex) const {
           size_t res = 0;
           hash_combine(res, vertex.pos);
           hash_combine(res, vertex.color);
           hash_combine(res, vertex.texCoord);
           return res;
        }
    };
}


namespace vk {
    ShaderModule::ShaderModule(VkDevice device, const std::string& filename) : m_Device(device) {
       std::vector<char> shaderCode = readFile(filename);
       m_CodeSize = shaderCode.size();
       std::cout << "[Vulkan] Loaded shader '" << filename << "' bytecode, bytes: " << m_CodeSize << std::endl;

       VkShaderModuleCreateInfo createInfo = {};
       createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
       createInfo.codeSize = m_CodeSize;
       createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

       if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &m_Module) != VK_SUCCESS)
          throw std::runtime_error("failed to create shader module!");
    }


    void Image::ChangeLayout(const CommandBuffer& cmdBuffer, VkImageLayout newLayout) {
       VkImageMemoryBarrier barrier = {};
       barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
       barrier.oldLayout = m_CurrentLayout;
       barrier.newLayout = newLayout;
       barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
       barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
       barrier.image = m_Image;
       barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
       barrier.subresourceRange.baseMipLevel = 0;
       barrier.subresourceRange.levelCount = m_MipLevels;
       barrier.subresourceRange.baseArrayLayer = 0;
       barrier.subresourceRange.layerCount = 1;
       barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

       VkPipelineStageFlags destinationStage;
       switch (m_CurrentLayout) {
          case VK_IMAGE_LAYOUT_UNDEFINED:
             barrier.srcAccessMask = 0;
             switch (newLayout) {
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                   barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                   m_CurrentStage = VK_PIPELINE_STAGE_HOST_BIT;
                   destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                   break;

                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                   barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                   if (hasStencilComponent(m_Format)) barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                   barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                   destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                   break;

                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                   barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                   destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                   break;

                default:
                   throw std::invalid_argument("unsupported layout transition!");
                   break;
             }
             break;

          case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
          case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
             if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                //barrier.srcQueueFamilyIndex = DeviceQueueIndices[QUEUE_TRANSFER];
                //barrier.dstQueueFamilyIndex = DeviceQueueIndices[QUEUE_GRAPHICS];

                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;

                //barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                //barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

                //destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
             } else {
                throw std::invalid_argument("unsupported layout transition!");
             }
             break;

          default:
             throw std::invalid_argument("unsupported layout transition!");
             break;
       }

       vkCmdPipelineBarrier(cmdBuffer.data(), m_CurrentStage, destinationStage, 0,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier
       );

       m_CurrentLayout = newLayout;
       m_CurrentStage = destinationStage;
    }


    void Image::ChangeLayout(const CommandBuffer& cmdBuffer, VkImageLayout newLayout, VkPipelineStageFlags dstStage, VkImageAspectFlags aspectFlags) {
       VkImageMemoryBarrier barrier = s_BaseBarrier;
       barrier.oldLayout = m_CurrentLayout;
       barrier.newLayout = newLayout;
       barrier.image = m_Image;
       barrier.subresourceRange.aspectMask = aspectFlags;
       barrier.subresourceRange.levelCount = m_MipLevels;
       barrier.subresourceRange.baseMipLevel = 0;
       barrier.subresourceRange.layerCount = 1;

       switch (m_CurrentLayout) {
          case VK_IMAGE_LAYOUT_UNDEFINED:
             barrier.srcAccessMask = 0;
             switch (newLayout) {
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                   barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                   m_CurrentStage = VK_PIPELINE_STAGE_HOST_BIT;
                   break;

                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                   barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                   if (hasStencilComponent(m_Format)) barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                   barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                   break;

                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                   barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                   break;

                default:
                   throw std::invalid_argument("unsupported layout transition!");
                   break;
             }
             break;

          case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
          case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
             if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
                if (dstStage == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) {
                   barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                } else {
                   barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
                }
             } else {
                throw std::invalid_argument("unsupported layout transition!");
             }
             break;

          default:
             throw std::invalid_argument("unsupported layout transition!");
             break;
       }

       vkCmdPipelineBarrier(cmdBuffer.data(), m_CurrentStage, dstStage, 0,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier
       );

       m_CurrentLayout = newLayout;
       m_CurrentStage = dstStage;
    }


    void Image::GenerateMipmaps(VkPhysicalDevice physicalDevice, const CommandBuffer& cmdBuffer) {
       VkFormatProperties formatProperties;
       vkGetPhysicalDeviceFormatProperties(physicalDevice, m_Format, &formatProperties);
       if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
          throw std::runtime_error("texture image format does not support linear blitting!");
       }

       if (m_CurrentLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
          throw std::runtime_error("image must be in 'VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL' layout when generating mipmaps!");

       VkImageMemoryBarrier barrier = {};
       barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
       barrier.image = m_Image;
       barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
       barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
       barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
       barrier.subresourceRange.baseArrayLayer = 0;
       barrier.subresourceRange.layerCount = 1;
       barrier.subresourceRange.levelCount = 1;

       int32_t mipWidth = Extent.width;
       int32_t mipHeight = Extent.height;
       for (uint32_t i = 1; i < m_MipLevels; i++) {
          barrier.subresourceRange.baseMipLevel = i - 1;
          barrier.oldLayout = m_CurrentLayout;
          barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
          barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
          barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

          vkCmdPipelineBarrier(cmdBuffer.data(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                               0, nullptr,
                               0, nullptr,
                               1, &barrier);

          VkImageBlit blit = {};
          blit.srcOffsets[0] = {0, 0, 0};
          blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
          blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
          blit.srcSubresource.mipLevel = i - 1;
          blit.srcSubresource.baseArrayLayer = 0;
          blit.srcSubresource.layerCount = 1;
          blit.dstOffsets[0] = {0, 0, 0};
          blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
          blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
          blit.dstSubresource.mipLevel = i;
          blit.dstSubresource.baseArrayLayer = 0;
          blit.dstSubresource.layerCount = 1;

          vkCmdBlitImage(cmdBuffer.data(),
                         m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1, &blit,
                         VK_FILTER_LINEAR);

          if (mipWidth > 1) mipWidth /= 2;
          if (mipHeight > 1) mipHeight /= 2;
       }

       barrier.subresourceRange.baseMipLevel = 0;
       barrier.subresourceRange.levelCount = m_MipLevels - 1;
       barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
       barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
       barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
       barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

       std::array<VkImageMemoryBarrier, 2> barriers = {barrier, barrier};
       barriers[1].subresourceRange.baseMipLevel = m_MipLevels - 1;
       barriers[1].subresourceRange.levelCount = 1;
       barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
       barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
       vkCmdPipelineBarrier(cmdBuffer.data(),
                            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                            0, nullptr,
                            0, nullptr,
                            barriers.size(), barriers.data());

       m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }


    Swapchain::Swapchain(VkDevice device, const std::set<uint32_t>& queueIndices, VkExtent2D extent, VkSurfaceKHR surface,
                         const VkSurfaceCapabilitiesKHR& capabilities,
                         const VkSurfaceFormatKHR& surfaceFormat, VkPresentModeKHR presentMode) : m_Device(device), m_Extent(extent) {

       uint32_t imageCount = capabilities.minImageCount + 1;
       if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
          imageCount = capabilities.maxImageCount;

       m_Format = surfaceFormat.format;

       VkSwapchainCreateInfoKHR createInfo = {};
       createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
       createInfo.surface = surface;
       createInfo.minImageCount = imageCount;
       createInfo.imageFormat = m_Format;
       createInfo.imageColorSpace = surfaceFormat.colorSpace;
       createInfo.imageExtent = m_Extent;
       createInfo.imageArrayLayers = 1;
       createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
       createInfo.preTransform = capabilities.currentTransform;
       createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
       createInfo.presentMode = presentMode;
       createInfo.clipped = VK_TRUE;
       createInfo.oldSwapchain = nullptr;

       if (queueIndices.size() < 2) {
          createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
          createInfo.queueFamilyIndexCount = 0;
          createInfo.pQueueFamilyIndices = nullptr;
       } else {
          std::vector<uint32_t> uniqueIndices(queueIndices.begin(), queueIndices.end());
          createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
          createInfo.queueFamilyIndexCount = uniqueIndices.size();
          createInfo.pQueueFamilyIndices = uniqueIndices.data();
       }

       if (vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
          throw std::runtime_error("Swapchain construction failed");

       if (vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr) != VK_SUCCESS)
          throw std::runtime_error("Swapchain image query failed");
       m_Images.resize(imageCount);
       if (vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_Images.data()) != VK_SUCCESS)
          throw std::runtime_error("Swapchain image query failed");


       for (VkImage image : m_Images)
          m_ImageViews.emplace_back(ImageView(m_Device, image, m_Format, 1, VK_IMAGE_ASPECT_COLOR_BIT));
    }


    Instance::Instance(bool enableValidation) {
       if (enableValidation && !checkValidationLayerSupport(ArrayToVector(s_ValidationLayers))) {
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
          createInfo.enabledLayerCount = s_ValidationLayers.size();
          createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
          debugCreateInfo = createDebugMsgInfo(debugCallback, nullptr);
          createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
       }

       if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
          throw std::runtime_error("Vulkan instance could not be created!");

       setupDebugMessenger();
    }


    void Instance::Release() noexcept {
       if (m_Instance) {
          if (m_DebugMessenger) DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
          vkDestroyInstance(m_Instance, nullptr);
       }
    }


    void Instance::setupDebugMessenger() {
       VkDebugUtilsMessengerCreateInfoEXT createInfo = createDebugMsgInfo(debugCallback, nullptr);
       if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
          throw std::runtime_error("Could not create Vulkan debug messenger");
    }


    Surface::Surface(VkInstance instance, GLFWwindow* window) : m_Instance(instance) {
       if (glfwCreateWindowSurface(m_Instance, window, nullptr, &m_Surface) != VK_SUCCESS) {
          throw std::runtime_error("failed to create window surface!");
       }
    }
}


Model::Model(const char* filepath) {
   std::ifstream dumpInput(std::string(filepath) + ".dump", std::ios::in | std::ios::binary);
   if (dumpInput) {
      /* Read header */
      std::size_t vertexCount;
      dumpInput.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
      m_Vertices.resize(vertexCount);
      dumpInput.read(reinterpret_cast<char*>(m_Vertices.data()), sizeof(Vertex) * vertexCount);

      std::size_t indexCount;
      dumpInput.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
      m_Indices.resize(indexCount);
      dumpInput.read(reinterpret_cast<char*>(m_Indices.data()), sizeof(VertexIndex) * indexCount);

      dumpInput.close();
   } else {
      tinyobj::attrib_t attrib;
      std::vector<tinyobj::shape_t> shapes;
      std::vector<tinyobj::material_t> materials;
      std::string warn, err;

      if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath)) {
         throw std::runtime_error(warn + err);
      }

      std::unordered_map<Vertex, uint32_t> uniqueVertices;
      for (const auto& shape : shapes) {
         for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};
            vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = {1.0f, 1.0f, 1.0f};

            auto it = uniqueVertices.find(vertex);
            if (it == uniqueVertices.cend()) {
               it = uniqueVertices.insert(std::make_pair(vertex, m_Vertices.size())).first;
               m_Vertices.push_back(vertex);
            }
            m_Indices.push_back(it->second);
         }
      }

      std::ofstream dumpOutput(std::string(filepath) + ".dump", std::ios::out | std::ios::binary);
      if (dumpOutput) {
         std::size_t vertexCount = VertexCount();
         std::size_t indexCount = IndexCount();
         dumpOutput.write(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
         dumpOutput.write(reinterpret_cast<char*>(m_Vertices.data()), VertexDataSize());
         dumpOutput.write(reinterpret_cast<char*>(&indexCount), sizeof(std::size_t));
         dumpOutput.write(reinterpret_cast<char*>(m_Indices.data()), IndexDataSize());
         dumpOutput.close();
      }
   }
}