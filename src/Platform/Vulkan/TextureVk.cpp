#include "TextureVk.h"
#include "Engine/Application.h"
#include "GraphicsContextVk.h"

using namespace vk;

void Texture2DVk::Upload() {
    auto &gfxContext = static_cast<GfxContextVk &>(Application::GetGraphicsContext());

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent.width = m_Width;
    imageInfo.extent.height = m_Height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = MipLevels();
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    Device &device = gfxContext.GetDevice();
    m_TextureImage = device.createImage(imageInfo);
    m_TextureMemory = device.allocateImageMemory({m_TextureImage}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_TextureImage->BindMemory(m_TextureMemory->data(), 0);
//    vkBindImageMemory(device, m_TextureImage->data(), m_TextureMemory->data(), 0);

    m_TextureView = device.createImageView(*m_TextureImage, VK_IMAGE_ASPECT_COLOR_BIT);
//   m_TextureView = m_TextureImage->createView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    // Create target image for copy
    StagingBuffer stagingBuffer(&device, m_Data.data(), m_Data.size());

    CommandPool pool(device, device.GfxQueueIdx());
    CommandBuffers setupCmdBuffers(device, pool.data());
    CommandBuffer setupCmdBuffer = setupCmdBuffers[0];
    setupCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    m_TextureImage->ChangeLayout(setupCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
    copyBufferToImage(setupCmdBuffer, stagingBuffer, *m_TextureImage);
    m_TextureImage->GenerateMipmaps(device, setupCmdBuffer);
    //m_TextureImage.ChangeLayout(setupCmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);


    setupCmdBuffer.End();
    setupCmdBuffer.Submit(device.GfxQueue());
    vkQueueWaitIdle(device.GfxQueue());
}


void TextureCubemapVk::Upload() {
    auto &gfxContext = static_cast<GfxContextVk &>(Application::GetGraphicsContext());

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent.width = m_Width;
    imageInfo.extent.height = m_Height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = MipLevels();
    imageInfo.arrayLayers = 6;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    Device &device = gfxContext.GetDevice();
    m_TextureImage = device.createImage(imageInfo);
    m_TextureMemory = device.allocateImageMemory({m_TextureImage}, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_TextureImage->BindMemory(m_TextureMemory->data(), 0);
//    vkBindImageMemory(device, m_TextureImage->data(), m_TextureMemory->data(), 0);

    m_TextureView = device.createImageView(*m_TextureImage, VK_IMAGE_ASPECT_COLOR_BIT);
//   m_TextureView = m_TextureImage->createView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    // Create target image for copy
    StagingBuffer stagingBuffer(&device, m_Data.data(), m_Data.size());

    CommandPool pool(device, device.GfxQueueIdx());
    CommandBuffers setupCmdBuffers(device, pool.data());
    CommandBuffer setupCmdBuffer = setupCmdBuffers[0];
    setupCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    m_TextureImage->ChangeLayout(setupCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
    copyBufferToImage(setupCmdBuffer, stagingBuffer, *m_TextureImage);
    m_TextureImage->GenerateMipmaps(device, setupCmdBuffer);


    setupCmdBuffer.End();
    setupCmdBuffer.Submit(device.GfxQueue());
    vkQueueWaitIdle(device.GfxQueue());
}
