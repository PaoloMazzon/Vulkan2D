/// \file Buffer.h
/// \author Paolo Mazzon
/// \brief Abstraction of VkBuffer to make handling them a little simpler
#pragma once
#include <vulkan/vulkan.h>
#include "VK2D/Structs.h"
#define VMA_VULKAN_VERSION 1002000
#include <VulkanMemoryAllocator/src/VmaUsage.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Creates a new buffer with memory and all that ready
/// \param dev Device to get the memory from (will use graphics indices)
/// \param size Size in bytes of the new buffer
/// \param usage How the buffer will be used
/// \param mem Required memory properties (device local, host visible, etc...)
/// \return Returns the new buffer or NULL if it failed
VK2DBuffer vk2dBufferCreate(VK2DLogicalDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem);

/// \brief Creates a buffer and loads some data into high-performance memory
/// \param dev Device to get the memory from
/// \param size Size in bytes of data
/// \param usage Usage of the buffer
/// \param data Data to put into high performance memory
/// \return Returns a new buffer with the data loaded or NULL if it failed
VK2DBuffer vk2dBufferLoad(VK2DLogicalDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, void *data, bool mainThread);

/// \brief Creates a buffer and loads 2 pieces of data into the same high-performance buffer
/// \param dev Device to get the memory from
/// \param size Size in bytes of data
/// \param usage Usage of the buffer
/// \param data Data to put into high performance memory
/// \param size2 Size of the 2nd piece of data that will be put into the same buffer
/// \param data2 Actual data2
/// \return Returns a new buffer with the data loaded or NULL if it failed
VK2DBuffer vk2dBufferLoad2(VK2DLogicalDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, void *data, VkDeviceSize size2, void *data2, bool mainThread);

/// \brief Copies the entire contents of src into dst
/// \param src Buffer to copy from
/// \param dst Buffer to copy to
/// \warning Both buffers must originate from the same device
void vk2dBufferCopy(VK2DBuffer src, VK2DBuffer dst, bool mainThread);

/// \brief Frees a buffer from memory
/// \param buf Buffer to free
void vk2dBufferFree(VK2DBuffer buf);

#ifdef __cplusplus
};
#endif