/// \file DescriptorBuffer.c
/// \author Paolo Mazzon
#include "VK2D/DescriptorBuffer.h"
#include "VK2D/Constants.h"
#include "VK2D/Buffer.h"
#include "VK2D/Validation.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/Renderer.h"

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
	VK2DDescriptorBuffer db = calloc(1, sizeof(struct VK2DDescriptorBuffer));
	vk2dPointerCheck(db);
	db->dev = vk2dRendererGetDevice();
	_vk2dDescriptorBufferAppendBuffer(db);
	VkEventCreateInfo eventCreateInfo = {VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};
	vk2dErrorCheck(vkCreateEvent(db->dev->dev, &eventCreateInfo, VK_NULL_HANDLE, &db->waitForCopyEvent));
	return db;
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
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	for (int i = 0; i < db->bufferCount; i++) {
		db->buffers[i].size = 0;
		vmaMapMemory(gRenderer->vma, db->buffers[i].stageBuffer->mem, &db->buffers[i].hostData);
	}

	vkResetEvent(db->dev->dev, db->waitForCopyEvent);
	vkCmdWaitEvents(
			drawBuffer,
			1,
			&db->waitForCopyEvent,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			0,
			VK_NULL_HANDLE,
			0,
			VK_NULL_HANDLE,
			0,
			VK_NULL_HANDLE);
}

void vk2dDescriptorBufferCopyData(VK2DDescriptorBuffer db, void *data, VkDeviceSize size, VkBuffer *outBuffer, VkDeviceSize *offset) {
	// TODO: Find a buffer with available space, creating a new one if not found, then copy the data to that buffer's stage, then copy the output data
}

void vk2dDescriptorBufferEndFrame(VK2DDescriptorBuffer db, VkCommandBuffer copyBuffer) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	for (int i = 0; i < db->bufferCount; i++) {
		vmaUnmapMemory(gRenderer->vma, db->buffers[i].stageBuffer->mem);
		// TODO: Copy this buffer if it has non-zero size
	}
	vkCmdSetEvent(copyBuffer, db->waitForCopyEvent, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
}
