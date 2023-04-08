/// \file DescriptorBuffer.c
/// \author Paolo Mazzon
#include "VK2D/DescriptorBuffer.h"
#include "VK2D/Constants.h"
#include "VK2D/Buffer.h"
#include "VK2D/Validation.h"
#include "VK2D/LogicalDevice.h"

static void _vk2dDescriptorBufferAppendBuffer(VK2DDescriptorBuffer db) {
	// Potentially increase the size of the buffer list
	if (db->bufferCount == db->bufferListSize) {
		db->buffers = realloc(db->buffers, sizeof(_VK2DDescriptorBufferInternal) * (db->bufferListSize + VK2D_DEFAULT_ARRAY_EXTENSION));
		vk2dPointerCheck(db->buffers);
		db->bufferListSize += VK2D_DEFAULT_ARRAY_EXTENSION;
	}

	// Find a spot in the buffer list for the new buffer
	_VK2DDescriptorBufferInternal *buffer = &db->buffers[db->bufferCount];
	db->bufferCount++;

	// Create the new buffers
	buffer->size = 0;
	buffer->stageBuffer = vk2dBufferCreate(
			db->dev,
			VK2D_DESCRIPTOR_BUFFER_INTERNAL_SIZE,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	buffer->deviceBuffer = vk2dBufferCreate(
			db->dev,
			VK2D_DESCRIPTOR_BUFFER_INTERNAL_SIZE,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

VK2DDescriptorBuffer vk2dDescriptorBufferCreate() {
	// TODO: Create the buffer memory, call _vk2dDescriptorBufferAppendBuffer on it, then create the event
	return NULL;
}

void vk2dDescriptorBufferFree(VK2DDescriptorBuffer db) {
	if (db != NULL) {
		for (int i = 0; i < db->bufferCount; i++) {
			vk2dBufferFree(db->buffers[i].deviceBuffer);
			vk2dBufferFree(db->buffers[i].stageBuffer);
		}
		free(db->buffers);
		vkDestroyEvent(db->dev->dev, db->waitForCopyEvent, VK_NULL_HANDLE);
		free(db);
	}
}

void vk2dDescriptorBufferBeginFrame(VK2DDescriptorBuffer db, VkCommandBuffer drawBuffer) {
	// TODO: Map stage buffer's memory, reset all of the buffers sizes to 0, queue the event wait in drawBuffer, and reset the event for this frame
}

void vk2dDescriptorBufferCopyData(VK2DDescriptorBuffer db, void *data, VkDeviceSize size, VkBuffer *outBuffer, VkDeviceSize *offset) {
	// TODO: Find a buffer with available space, creating a new one if not found, then copy the data to that buffer's stage, then copy the output data
}

void vk2dDescriptorBufferEndFrame(VK2DDescriptorBuffer db, VkCommandBuffer copyBuffer) {
	// TODO: Unmap memory, queue the copy command for each buffer with > 0 size in copyBuffer, and set off the event after the command
}
