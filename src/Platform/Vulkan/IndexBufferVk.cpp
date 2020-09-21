#include <Engine/Renderer/Renderer.h>
#include "IndexBufferVk.h"

void IndexBufferVk::Bind(VkCommandBuffer cmdBuffer, uint64_t offset) const {
    vkCmdBindIndexBuffer(cmdBuffer, m_Buffer.buffer(), offset, VK_INDEX_TYPE_UINT32);
}

void IndexBufferVk::UploadData(const void *data, uint64_t bytes, uint64_t indexCount) {
//    m_IndexCount = indexCount;
//
//    std::array<VkBufferMemoryBarrier, 1> bufferBarriers = {};
//    vk::Semaphore transferSemaphore(m_Device);
//    VkSubmitInfo submitInfo = {};
//    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//
//    vk::CommandBuffers transferCmdBuffers(m_Device, m_Device.TransferPool()->data());
//    {
//
//        vk::CommandBuffer transferCmdBuffer = transferCmdBuffers[0];
//        transferCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
//
//        m_Buffer = DeviceBuffer(&m_Device, data, bytes, transferCmdBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
//
//        bufferBarriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
//        bufferBarriers[0].srcQueueFamilyIndex = m_Device.queueIndex(QueueFamily::TRANSFER);
//        bufferBarriers[0].dstQueueFamilyIndex = m_Device.queueIndex(QueueFamily::GRAPHICS);
//        bufferBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//        bufferBarriers[0].dstAccessMask = 0;
//        bufferBarriers[0].buffer = m_Buffer.data().data();
//        bufferBarriers[0].size = m_Buffer.size();
//        bufferBarriers[0].offset = 0;
//
//        vkCmdPipelineBarrier(transferCmdBuffer.data(),
//                             VK_PIPELINE_STAGE_TRANSFER_BIT,
//                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
//                             0, nullptr,
//                             bufferBarriers.size(), bufferBarriers.data(),
//                             0, nullptr
//        );
//
//        transferCmdBuffer.End();
//
//        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//        submitInfo.signalSemaphoreCount = 1;
//        submitInfo.pSignalSemaphores = transferSemaphore.ptr();
//        transferCmdBuffer.Submit(submitInfo, m_Device.queue(QueueFamily::TRANSFER));
//    }
//
//    /* Acquisition phase */
//    {
//        vk::CommandBuffers cmds(m_Device, m_Device.GfxPool()->data());
//        vk::CommandBuffer gfxCmdBuffer = cmds[0];
//        gfxCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
//
//        bufferBarriers[0].srcAccessMask = 0;
//        bufferBarriers[0].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
//        vkCmdPipelineBarrier(gfxCmdBuffer.data(),
//                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
//                             VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
//                             0, nullptr,
//                             bufferBarriers.size(), bufferBarriers.data(),
//                             0, nullptr
//        );
//
//        gfxCmdBuffer.End();
//
//        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//        submitInfo.waitSemaphoreCount = 1;
//        submitInfo.pWaitSemaphores = transferSemaphore.ptr();
//        submitInfo.pWaitDstStageMask = &waitStage;
//        gfxCmdBuffer.Submit(submitInfo, m_Device.queue(QueueFamily::GRAPHICS));
//
//        vkQueueWaitIdle(m_Device.queue(QueueFamily::GRAPHICS));
//    }
}

void IndexBufferVk::StageData(const void *data, uint64_t bytes, uint64_t indexCount) {
    m_IndexCount = indexCount;
    Renderer::StageData(&m_BufferHandle, &m_BufferOffset, data, bytes);
}
