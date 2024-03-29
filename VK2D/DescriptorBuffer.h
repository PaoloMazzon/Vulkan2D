/// \file DescriptorBuffer.h
/// \author Paolo Mazzon
/// \brief Tool to automate memory for uniform buffers and such
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VK2D/Structs.h"

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
/// \warning size ***MUST*** be less than VK2D_DESCRIPTOR_BUFFER_INTERNAL_SIZE (which is 250kb by default)
void vk2dDescriptorBufferCopyData(VK2DDescriptorBuffer db, void *data, VkDeviceSize size, VkBuffer *outBuffer, VkDeviceSize *offset);

/// \brief Finishes tasks that need to be done in command buffers before the queue is submitted
/// \param db Descriptor buffer to finish the frame on
/// \param copyBuffer A (likely new) command buffer in recording state that will have the memory copy placed into it
void vk2dDescriptorBufferEndFrame(VK2DDescriptorBuffer db, VkCommandBuffer copyBuffer);

#ifdef __cplusplus
}
#endif