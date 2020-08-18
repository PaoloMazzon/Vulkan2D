/// \file Buffer.h
/// \author Paolo Mazzon
/// \brief Abstraction of VkBuffer to make handling them a little simpler
#pragma once
#include <vulkan/vulkan.h>
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Makes managing buffers in Vulkan simpler
struct VK2DBuffer {
	VkBuffer buf;          ///< Internal Vulkan buffer
	VkDeviceMemory mem;    ///< Memory for the buffer
	VK2DLogicalDevice dev; ///< Device the buffer belongs to
	VkDeviceSize size;     ///< Size of this buffer in bytes
};

/// \brief Creates a new buffer with memory and all that ready
/// \param size Size in bytes of the new buffer
/// \param usage How the buffer will be used
/// \param mem Required memory properties (device local, host visible, etc...)
/// \param dev Device to get the memory from (will use graphics indices)
/// \return Returns the new buffer or NULL if it failed
VK2DBuffer vk2dBufferCreate(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem, VK2DLogicalDevice dev);

/// \brief Copies the entire contents of src into dst
/// \param src Buffer to copy from
/// \param dst Buffer to copy to
/// \param commandPool Index of the command pool to use on the previously assigned device
/// \warning Both buffers must originate from the same device
void vk2dBufferCopy(VK2DBuffer src, VK2DBuffer dst, uint32_t pool);

/// \brief Frees a buffer from memory
/// \param buf Buffer to free
void vk2dBufferFree(VK2DBuffer buf);

#ifdef __cplusplus
};
#endif