/// \file VulkanInterface.h
/// \author Paolo Mazzon
/// \brief Tools to allow the user to mess with vulkan themselves
#pragma once
#include "VK2D/Structs.h"
#include <VulkanMemoryAllocator/src/VmaUsage.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Returns a command buffer intended to be used once
/// \return Returns a new command buffer in the recording state
VkCommandBuffer vk2dVulkanGetSingleUseBuffer();

/// \brief Submits a command buffer previously acquired with vk2dVulkanGetSingleUseBuffer and waits for the queue to finish
/// \param buffer Command buffer previously acquired with vk2dVulkanGetSingleUseBuffer
void vk2dVulkanSubmitSingleUseBuffer(VkCommandBuffer buffer);

/// \brief Returns the command buffer being used to draw the active frame
/// \return Returns a command buffer in the recording state
/// \warning This is only valid until the next time another VK2D function is called
/// \warning The draw buffer is guaranteed to be in a render pass but there is no guarantee on which one
VkCommandBuffer vk2dVulkanGetDrawBuffer();

/// \brief Copies arbitrary data into a device-local buffer that can then be accessed from command buffers
/// \param data Data to copy to the gpu
/// \param size Size of the data in bytes
/// \param outBuffer Device-local Vulkan buffer that the data will be available on
/// \param bufferOffset Offset in outBuffer data can be accessed from, will follow uniform buffer alignment rules
/// \warning You may not pass data of size larger than the vramPageSize specified when you initialized the renderer
/// \warning The buffer the data is copied into is only intended for uniform buffers and vertex buffers
/// \warning Data copied into a buffer like this is only valid for a single frame; you must use this every frame
void vk2dVulkanCopyDataIntoBuffer(void *data, VkDeviceSize size, VkBuffer *outBuffer, VkDeviceSize *bufferOffset);

/// \brief Returns the current frame being rendered
/// \return Returns the current frame being rendered
int vk2dVulkanGetFrame();

/// \brief Returns the current swapchain image being used this frame
/// \return Returns the current swapchain image being used this frame
int vk2dVulkanGetSwapchainImageIndex();

/// \brief Returns the logical device
/// \return Returns the logical device
VkDevice vk2dVulkanGetDevice();

/// \brief Returns the renderer-selected physical device
/// \return Returns the renderer-selected physical device
VkPhysicalDevice vk2dVulkanGetPhysicalDevice();

/// \brief Returns the queue being used for everything
/// \return Returns the queue being used for everything
VkQueue vk2dVulkanGetQueue();

/// \brief Returns the queue family in use
/// \return Returns the queue family in use
uint32_t vk2dVulkanGetQueueFamily();

/// \brief Returns the swapchain image count
/// \return Returns the swapchain image count
uint32_t vk2dVulkanGetSwapchainImageCount();

/// \brief Returns the maximum number of frames in flight allowed at once
/// \return Returns the maximum number of frames in flight allowed at once
uint32_t vk2dVulkanGetMaxFramesInFlight();

/// \brief Returns the VMA instance
/// \return Returns the VMA instance
VmaAllocator vk2dVulkanGetVMA();

#ifdef __cplusplus
}
#endif