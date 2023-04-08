/// \file DescriptorBuffer.h
/// \author Paolo Mazzon
/// \brief Tool to automate memory for uniform buffers and such
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VK2D/Structs.h"

/// \brief To make descriptor buffers simpler internally
typedef struct _VK2DDescriptorBufferInternal {
	VK2DBuffer deviceBuffer; ///< Device-local (on vram) buffer that the shaders will access
	VK2DBuffer stageBuffer;  ///< Host-local (on ram) buffer that data will be copied into
	VkDeviceSize size;       ///< Amount of data currently in this buffer
} _VK2DDescriptorBufferInternal;

/// \brief Automates memory management for uniform buffers and the lot
///
/// The intended usage is as follows:
///
///  1. Create the descriptor buffer
///  2. Call vk2dDescriptorBufferBeginFrame before beginning to draw
///  3.
///  4. Call vk2dDescriptorBufferEndFrame before submitting the queue
///
/// It will put a event barrier into the startframe command buffer where drawing happens,
/// and then it will put a memory copy into the second command buffer at the end of the
/// frame so Vulkan copies the buffer to device memory all at once instead of tiny increments.
struct VK2DDescriptorBuffer {
	_VK2DDescriptorBufferInternal *buffers; ///< List of internal buffers so that we can allocate more on the fly
	int bufferCount;     ///< Amount of internal buffers in the descriptor buffer, for when it needs to be resized
	int bufferListSize;  ///< Actual number of elements in the buffer lists
	VkEvent waitForCopy; ///< Vulkan event for synchronizing the buffer copy across command buffers
};

/// \brief Creates an empty descriptor buffer of default size
/// \return Returns a new descriptor buffer or NULL if it fails
VK2DDescriptorBuffer vk2dDescriptorBufferCreate();

/// \brief Frees a descriptor buffer from memory
/// \param db Descriptor buffer to free
void vk2dDescriptorBufferFree(VK2DDescriptorBuffer db);

/// \brief Prepares the buffer for copying
/// \param db Descriptor buffer to prepare
/// \param drawBuffer Command buffer that will be used for drawing, in recording state
void vk2dDescriptorBufferBeginFrame(VK2DDescriptorBuffer db, VkCommandBuffer drawBuffer);

/// \brief Adds data to the buffer, resizing itself if it needs to
/// \param db Descriptor buffer to add data to
/// \param data Pointer to the data to add
/// \param size Size in bytes of the data
/// \param outBuffer Will be filled with the pointer to the internal Vulkan buffer that the memory is located in
/// \param offset Location in outBuffer where the copied data is
void vk2dDescriptorBufferCopyData(VK2DDescriptorBuffer db, void *data, VkDeviceSize size, VkBuffer *outBuffer, VkDeviceSize *offset);

/// \brief Finishes tasks that need to be done in command buffers before the queue is submitted
/// \param db Descriptor buffer to finish the frame on
/// \param copyBuffer A (likely new) command buffer in recording state that will have the memory copy placed into it
void vk2dDescriptorBufferEndFrame(VK2DDescriptorBuffer db, VkCommandBuffer copyBuffer);

#ifdef __cplusplus
}
#endif