/// \file DescriptorBuffer.c
/// \author Paolo Mazzon
#include "VK2D/DescriptorBuffer.h"
#include "VK2D/Constants.h"
#include "VK2D/Buffer.h"
#include "VK2D/Validation.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/Renderer.h"
#include "VK2D/Initializers.h"
#include "VK2D/PhysicalDevice.h"

static _VK2DDescriptorBufferInternal *_vk2dDescriptorBufferAppendBuffer(VK2DDescriptorBuffer db) {
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

	return buffer;
}

VK2DDescriptorBuffer vk2dDescriptorBufferCreate() {
	// TODO: Create the buffer memory, call _vk2dDescriptorBufferAppendBuffer on it, then create the event
	VK2DDescriptorBuffer db = calloc(1, sizeof(struct VK2DDescriptorBuffer));
	vk2dPointerCheck(db);
	db->dev = vk2dRendererGetDevice();
	_vk2dDescriptorBufferAppendBuffer(db);
	return db;
}

void vk2dDescriptorBufferFree(VK2DDescriptorBuffer db) {
	if (db != NULL) {
		for (int i = 0; i < db->bufferCount; i++) {
			vk2dBufferFree(db->buffers[i].deviceBuffer);
			vk2dBufferFree(db->buffers[i].stageBuffer);
		}
		free(db->buffers);
		free(db);
	}
}

void vk2dDescriptorBufferBeginFrame(VK2DDescriptorBuffer db, VkCommandBuffer drawBuffer) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	for (int i = 0; i < db->bufferCount; i++) {
		db->buffers[i].size = 0;
		vmaMapMemory(gRenderer->vma, db->buffers[i].stageBuffer->mem, &db->buffers[i].hostData);
	}

	vkCmdPipelineBarrier(
			drawBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			0,
			0,
			VK_NULL_HANDLE,
			0,
			VK_NULL_HANDLE,
			0,
			VK_NULL_HANDLE
	);
}

void vk2dDescriptorBufferCopyData(VK2DDescriptorBuffer db, void *data, VkDeviceSize size, VkBuffer *outBuffer, VkDeviceSize *offset) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (size < VK2D_DESCRIPTOR_BUFFER_INTERNAL_SIZE) {
		// Find a buffer with enough space
		_VK2DDescriptorBufferInternal *spot = NULL;
		for (int i = 0; i < db->bufferCount && spot == NULL; i++) {
			if (size < VK2D_DESCRIPTOR_BUFFER_INTERNAL_SIZE - db->buffers[i].size) {
				spot = &db->buffers[i];
			}
		}

		// If no buffer exists, make a new one
		if (spot == NULL) {
			spot = _vk2dDescriptorBufferAppendBuffer(db);
			spot->size = 0;
			vmaMapMemory(gRenderer->vma, spot->stageBuffer->mem, &spot->hostData);
		}

		// Copy data over
		memcpy(spot->hostData + spot->size, data, size);
		*outBuffer = spot->deviceBuffer->buf;
		*offset = spot->size;

		// We may only move size in accordance with minUniformBufferOffsetAlignment
		if (size % gRenderer->pd->props.limits.minUniformBufferOffsetAlignment != 0) {
			spot->size += ((size / gRenderer->pd->props.limits.minUniformBufferOffsetAlignment) + 1) * gRenderer->pd->props.limits.minUniformBufferOffsetAlignment;
		} else {
			spot->size += size;
		}
	}
}

void vk2dDescriptorBufferEndFrame(VK2DDescriptorBuffer db, VkCommandBuffer copyBuffer) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();

	// Unmap all of the buffers then queue a buffer copy if their size is greater than 0
	for (int i = 0; i < db->bufferCount; i++) {
		vmaUnmapMemory(gRenderer->vma, db->buffers[i].stageBuffer->mem);
		if (db->buffers[i].size > 0) {
			VkBufferCopy bufferCopy = {};
			bufferCopy.size = (db->buffers[i].size < VK2D_DESCRIPTOR_BUFFER_INTERNAL_SIZE) ? db->buffers[i].size : VK2D_DESCRIPTOR_BUFFER_INTERNAL_SIZE;
			vkCmdCopyBuffer(copyBuffer, db->buffers[i].stageBuffer->buf, db->buffers[i].deviceBuffer->buf, 1, &bufferCopy);
		}
	}
}
