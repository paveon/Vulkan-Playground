#include "Device.h"

#include <iostream>
#include <set>
#include <sstream>
#include <algorithm>
#include <numeric>

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

                for (uint32_t i = VK_SAMPLE_COUNT_2_BIT; i <= VK_SAMPLE_COUNT_64_BIT; i <<= 1u) {
                    if (counts & i) m_SupportedSamplesMSAA.emplace_back((VkSampleCountFlagBits)i);
                }
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


auto RingStageBuffer::PopMetadata() -> RingStageBuffer::DataInfo {
    DataInfo info = m_Metadata.front();
    if ((m_StartOffset + info.dataSize) >= m_Size) {
        m_StartOffset = (m_StartOffset + info.dataSize) - m_Size;
    } else {
        m_StartOffset += info.dataSize;
    }
    m_Metadata.pop();
    if (m_Metadata.empty()) {
        m_StartOffset = 0;
        m_EndOffset = 0;
    }

    return info;
}


void RingStageBuffer::Allocate(VkDeviceSize size) {
    if (m_Memory) {
        if (size > m_Size) {
            VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            vk::Buffer *newBuffer = m_Device->createBuffer({m_Device->TransferQueueIdx()},
                                                           size,
                                                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

            vk::DeviceMemory *newMemory = m_Device->allocateBufferMemory(*newBuffer, memoryFlags);
            newBuffer->BindMemory(newMemory->data(), 0);
            newMemory->MapMemory(0, size);

            void *srcPtr = (uint8_t *) m_Memory->m_Mapped + m_StartOffset;
            if (m_EndOffset > m_StartOffset) {
                VkDeviceSize range = m_EndOffset - m_StartOffset;
                std::memcpy(newMemory->m_Mapped, srcPtr, range);
            } else {
                VkDeviceSize range = m_Size - m_StartOffset;
                void *dstPtr = newMemory->m_Mapped;
                std::memcpy(dstPtr, srcPtr, range);
                dstPtr = (uint8_t *) dstPtr + range;
                srcPtr = m_Memory->m_Mapped;
                range = m_EndOffset;
                std::memcpy(dstPtr, srcPtr, range);
            }
            m_StartOffset = 0;
            m_EndOffset = m_Size;
            m_Size = size;
            m_Data = newBuffer;
            m_Memory = newMemory;
        }
    } else {
        VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        m_Size = size;
        m_Data = m_Device->createBuffer({m_Device->TransferQueueIdx()},
                                        m_Size,
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        m_Memory = m_Device->allocateBufferMemory(*m_Data, memoryFlags);
        m_Data->BindMemory(m_Memory->data(), 0);
        m_Memory->MapMemory(0, m_Size);
    }
}


//void RingStageBuffer::StageData(vk::Buffer **dstHandlePtr,
//                                VkDeviceSize *dstOffsetHandlePtr,
//                                const void *data,
//                                VkDeviceSize dataSize) {
//    VkDeviceSize freeSpace = m_EndOffset > m_StartOffset ?
//                             m_Size + m_StartOffset - m_EndOffset : m_Size - m_StartOffset + m_EndOffset;
//
//    if (dataSize >= FreeSpace())
//        throw std::runtime_error("[RingStageBuffer] Not enough free space");
//
//    m_Metadata.push(DataInfo{
//            {},
//            m_EndOffset,
//            dataSize,
//            DataType::MESH_DATA,
//            0
//    });
//
//    if (m_EndOffset + dataSize > m_Size) {
//        void *bufferPtr = (uint8_t *) m_Memory->m_Mapped + m_EndOffset;
//        const void *dataPtr = data;
//        VkDeviceSize chunkSize = m_Size - m_EndOffset;
//        std::memcpy(bufferPtr, dataPtr, chunkSize);
//        dataPtr = (uint8_t *) dataPtr + chunkSize;
//        m_EndOffset = dataSize - chunkSize;
//        std::memcpy(m_Memory->m_Mapped, dataPtr, m_EndOffset);
//    } else {
//        std::memcpy((uint8_t *) m_Memory->m_Mapped + m_EndOffset, data, dataSize);
//        m_EndOffset += dataSize;
//    }
//}


void RingStageBuffer::StageMesh(const Mesh *mesh) {
    const auto &vertexData = mesh->VertexData();
    const auto &indices = mesh->Indices();
    auto indexDataSize = indices.size() * sizeof(uint32_t);
    VkDeviceSize dataSize = vertexData.size() + indexDataSize;
    if (dataSize >= FreeSpace())
        throw std::runtime_error("[RingStageBuffer] Not enough free space");

    m_Metadata.push(DataInfo{
            {},
            m_EndOffset,
            dataSize,
            DataType::MESH_DATA,
            mesh->MeshID()
    });
    std::vector<VkBufferCopy> &regions = m_Metadata.back().copyRegions;
    if (m_EndOffset + dataSize > m_Size) {
        uint8_t *bufferPtr = (uint8_t *) m_Memory->m_Mapped + m_EndOffset;
        VkDeviceSize chunkSize = m_Size - m_EndOffset;

        regions.emplace_back(VkBufferCopy{m_EndOffset, 0, chunkSize});
        regions.emplace_back(VkBufferCopy{0, 0, dataSize - chunkSize});

        if (chunkSize > vertexData.size()) {
            std::memcpy(bufferPtr, vertexData.data(), vertexData.size());
            bufferPtr += vertexData.size();

            std::memcpy(bufferPtr, indices.data(), chunkSize - vertexData.size());
            const uint8_t *dataPtr = (uint8_t *) indices.data() + (chunkSize - vertexData.size());
            m_EndOffset = dataSize - chunkSize;
            assert(m_EndOffset == (indexDataSize - (chunkSize - vertexData.size())));
            std::memcpy(m_Memory->m_Mapped, dataPtr, m_EndOffset);
        } else if (chunkSize < vertexData.size()) {
            std::memcpy(bufferPtr, vertexData.data(), chunkSize);
            const uint8_t *dataPtr = (uint8_t *) vertexData.data() + chunkSize;
            m_EndOffset = dataSize - chunkSize;
            assert(m_EndOffset == (vertexData.size() - chunkSize) + indexDataSize);
            bufferPtr = (uint8_t *) m_Memory->m_Mapped;
            std::memcpy(bufferPtr, dataPtr, vertexData.size() - chunkSize);
            bufferPtr += (vertexData.size() - chunkSize);
            std::memcpy(bufferPtr, indices.data(), indexDataSize);
        } else {
            std::memcpy(bufferPtr, vertexData.data(), chunkSize);
            m_EndOffset = dataSize - chunkSize;
            assert(m_EndOffset == indexDataSize);
            std::memcpy(m_Memory->m_Mapped, indices.data(), m_EndOffset);
        }
    } else {
        regions.emplace_back(VkBufferCopy{m_EndOffset, 0, dataSize});
        uint8_t *bufferPtr = (uint8_t *) m_Memory->m_Mapped + m_EndOffset;
        std::memcpy(bufferPtr, vertexData.data(), vertexData.size());
        bufferPtr += vertexData.size();
        std::memcpy(bufferPtr, indices.data(), indexDataSize);
        m_EndOffset += dataSize;
    }
}


void DeviceBuffer::Allocate(VkDeviceSize size, VkBufferUsageFlags usage) {
    std::set<uint32_t> indices{m_Device->queueIndex(QueueFamily::TRANSFER),
                               m_Device->queueIndex(QueueFamily::GRAPHICS)};
    m_Buffer = vk::Buffer(*m_Device, indices, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(*m_Device, m_Buffer.data(), &memRequirements);
    auto memoryTypeIdx = m_Device->getMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_BufferMemory = vk::DeviceMemory(*m_Device, memoryTypeIdx, size);
    m_Buffer.BindMemory(m_BufferMemory.data(), 0);
    m_SubAllocations.emplace_back(Metadata{0, size, true});
}


auto DeviceBuffer::TransferData(const vk::CommandBuffer &cmdBuffer,
                                const RingStageBuffer &stageBuffer,
                                std::vector<VkBufferCopy> &copyRegions) -> VkBufferCopy {
    assert(!copyRegions.empty());

    size_t bytes = std::accumulate(copyRegions.begin(), copyRegions.end(), 0,
                                   [](const size_t &x, const VkBufferCopy &y) {
                                       return x + y.size;
                                   });

    /// Find bestfit block
    auto bestfitIt = m_SubAllocations.end();
    for (auto it = m_SubAllocations.begin(); it != m_SubAllocations.end(); ++it) {
        if (it->free && it->size >= bytes) {
            if (bestfitIt != m_SubAllocations.end()) {
                if (it->size < bestfitIt->size) bestfitIt = it;
            } else {
                bestfitIt = it;
            }
        }
    }

    if (bestfitIt == m_SubAllocations.end()) {
        std::ostringstream msg;
        msg << "[DeviceBuffer (" << m_Buffer.ptr() << ")] Not enough free space";
        throw std::runtime_error(msg.str().c_str());
    }

    if (bestfitIt->size > bytes) {
        bestfitIt = m_SubAllocations.emplace(bestfitIt + 1, Metadata{
                bestfitIt->offset + bytes,
                bestfitIt->size - bytes,
                true
        });
        bestfitIt--;
    }
    bestfitIt->free = false;
    bestfitIt->size = bytes;

    for (auto &region : copyRegions) {
        region.dstOffset += bestfitIt->offset;
    }
    vkCmdCopyBuffer(cmdBuffer.data(),
                    stageBuffer.buffer(),
                    m_Buffer.data(),
                    copyRegions.size(),
                    copyRegions.data());

    return VkBufferCopy{
            copyRegions[0].srcOffset,
            bestfitIt->offset,
            bestfitIt->size
    };
}


void vk::UniformBuffer::Allocate(VkDeviceSize size) {
    auto minOffset = m_Device->properties().limits.minUniformBufferOffsetAlignment;
    size = roundUp(size, minOffset);

    VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    m_Buffer = vk::Buffer(*m_Device,
                          {m_Device->GfxQueueIdx()},
                          size,
                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(*m_Device, m_Buffer.data(), &memRequirements);
    auto memoryType = m_Device->getMemoryType(memRequirements.memoryTypeBits, memoryFlags);
    m_BufferMemory = vk::DeviceMemory(*m_Device, memoryType, memRequirements.size);
    m_Buffer.BindMemory(m_BufferMemory.data(), 0);
    m_SubAllocations.emplace_back(Metadata{0, size, true});
}


auto vk::UniformBuffer::SubAllocate(VkDeviceSize size) -> VkDeviceSize {
    /// Assuming requested size is correctly aligned

    auto bestfitIt = m_SubAllocations.end();
    for (auto it = m_SubAllocations.begin(); it != m_SubAllocations.end(); ++it) {
        if (it->free && it->size >= size) {
            if (bestfitIt != m_SubAllocations.end()) {
                if (it->size < bestfitIt->size) bestfitIt = it;
            } else {
                bestfitIt = it;
            }
        }
    }

    if (bestfitIt == m_SubAllocations.end()) {
        std::ostringstream msg;
        msg << "[UniformBuffer (" << m_Buffer.ptr() << ")] Not enough free space";
        throw std::runtime_error(msg.str().c_str());
    }

    if (bestfitIt->size > size) {
        bestfitIt = m_SubAllocations.emplace(bestfitIt + 1, Metadata{
                bestfitIt->offset + size,
                bestfitIt->size - size,
                true
        });
        bestfitIt--;
    }
    bestfitIt->free = false;
    bestfitIt->size = size;

    return bestfitIt->offset;
}
