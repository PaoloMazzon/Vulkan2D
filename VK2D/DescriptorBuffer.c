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
#include "VK2D/Opaque.h"

static _VK2DDescriptorBufferInternal *_vk2dDescriptorBufferAppendBuffer(VK2DDescriptorBuffer db) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal() || gRenderer == NULL)
        return NULL;
	// Potentially increase the size of the buffer list
	if (db->bufferCount == db->bufferListSize) {
		db->buffers = realloc(db->buffers, sizeof(_VK2DDescriptorBufferInternal) * (db->bufferListSize + VK2D_DEFAULT_ARRAY_EXTENSION));
		db->memoryBarriers = realloc(db->memoryBarriers, sizeof(VkBufferMemoryBarrier) * (db->bufferListSize + VK2D_DEFAULT_ARRAY_EXTENSION));
		db->bufferListSize += VK2D_DEFAULT_ARRAY_EXTENSION;
		if (db->buffers == NULL || db->memoryBarriers == NULL) {
		    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to reallocate buffers list.");
		}
	}

	if (vk2dStatusFatal())
	    return NULL;

	// Find a spot in the buffer list for the new buffer
	_VK2DDescriptorBufferInternal *buffer = &db->buffers[db->bufferCount];
	db->bufferCount++;

	// Create the new buffers
	buffer->size = 0;
	buffer->stageBuffer = vk2dBufferCreate(
			db->dev,
			db->pageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	buffer->deviceBuffer = vk2dBufferCreate(
			db->dev,
			db->pageSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (buffer->stageBuffer == NULL || buffer->deviceBuffer == NULL) {
        db->bufferCount--;
        vk2dBufferFree(buffer->stageBuffer);
        vk2dBufferFree(buffer->deviceBuffer);
        return NULL;
    }

	return buffer;
}

VK2DDescriptorBuffer vk2dDescriptorBufferCreate(VkDeviceSize vramPageSize) {
    if (vk2dStatusFatal() || vk2dRendererGetPointer() == NULL)
        return NULL;
	VK2DDescriptorBuffer db = calloc(1, sizeof(struct VK2DDescriptorBuffer_t));
	if (db == NULL) {
	    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate descriptor buffer.");
	    return NULL;
	}

	db->dev = vk2dRendererGetDevice();
	db->pageSize = vramPageSize;
	if (_vk2dDescriptorBufferAppendBuffer(db) == NULL) {
	    free(db);
	    return NULL;
	}
	return db;
}

void vk2dDescriptorBufferFree(VK2DDescriptorBuffer db) {
    if (vk2dStatusFatal() || vk2dRendererGetPointer() == NULL)
        return;
	if (db != NULL) {
		for (int i = 0; i < db->bufferCount; i++) {
			vk2dBufferFree(db->buffers[i].deviceBuffer);
			vk2dBufferFree(db->buffers[i].stageBuffer);
		}
		free(db->buffers);
		free(db->memoryBarriers);
		free(db);
	}
}

void vk2dDescriptorBufferBeginFrame(VK2DDescriptorBuffer db, VkCommandBuffer copyCommandBuffer) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (vk2dStatusFatal() || gRenderer == NULL)
        return;
	db->copyCommandBuffer = copyCommandBuffer;

	for (int i = 0; i < db->bufferCount; i++) {
        // Map this buffer to ram
		db->buffers[i].size = 0;
		VkResult result = vmaMapMemory(gRenderer->vma, db->buffers[i].stageBuffer->mem, &db->buffers[i].hostData);
		if (result != VK_SUCCESS) {
            vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to map memory, VMA error %i.", result);
            return;
		}
	}
}

static VkDeviceSize maxTwo(VkDeviceSize s1, VkDeviceSize s2) {
    return s1 > s2 ? s1 : s2;
}

void vk2dDescriptorBufferCopyData(VK2DDescriptorBuffer db, void *data, VkDeviceSize size, VkBuffer *outBuffer, VkDeviceSize *offset) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    *outBuffer = VK_NULL_HANDLE;
    *offset = 0;
    if (vk2dStatusFatal() || gRenderer == NULL)
        return;

    if (size < db->pageSize) {
        // Find a buffer with enough space
        _VK2DDescriptorBufferInternal *spot = NULL;
        for (int i = 0; i < db->bufferCount && spot == NULL; i++) {
            if (size <= db->pageSize - db->buffers[i].size) {
                spot = &db->buffers[i];
            }
        }

        // If no buffer exists, make a new one
        if (spot == NULL) {
            spot = _vk2dDescriptorBufferAppendBuffer(db);
            if (spot != NULL) {
                spot->size = 0;
                VkResult result = vmaMapMemory(gRenderer->vma, spot->stageBuffer->mem, &spot->hostData);
                if (result != VK_SUCCESS) {
                    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to map memory, VMA error %i.", result);
                    return;
                }
            } else {
                return;
            }
        }

        // Copy data over
        uint8_t *np = spot->hostData;
        memcpy(np + spot->size, data, size);
        *outBuffer = spot->deviceBuffer->buf;
        *offset = spot->size;

        // We may only move size in accordance with minUniformBufferOffsetAlignment
        const VkDeviceSize alignment = maxTwo(gRenderer->pd->props.limits.minStorageBufferOffsetAlignment, gRenderer->pd->props.limits.minUniformBufferOffsetAlignment);
        if (size % alignment != 0) {
            spot->size += ((size / alignment) + 1) * alignment;
        } else {
            spot->size += size;
        }
    }
}

void vk2dDescriptorBufferReserveSpace(VK2DDescriptorBuffer db, VkDeviceSize size, VkBuffer *outBuffer, VkDeviceSize *offset) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    *outBuffer = VK_NULL_HANDLE;
    *offset = 0;
    if (vk2dStatusFatal() || gRenderer == NULL)
        return;

    if (size < db->pageSize) {
        // Find a buffer with enough space
        _VK2DDescriptorBufferInternal *spot = NULL;
        for (int i = 0; i < db->bufferCount && spot == NULL; i++) {
            if (size <= db->pageSize - db->buffers[i].size) {
                spot = &db->buffers[i];
            }
        }

        // If no buffer exists, make a new one
        if (spot == NULL) {
            spot = _vk2dDescriptorBufferAppendBuffer(db);
            if (spot != NULL) {
                spot->size = 0;
                VkResult result = vmaMapMemory(gRenderer->vma, spot->stageBuffer->mem, &spot->hostData);
                if (result != VK_SUCCESS) {
                    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to map memory, VMA error %i.", result);
                    return;
                }
            } else {
                return;
            }
        }

        // Copy data over
        *outBuffer = spot->deviceBuffer->buf;
        *offset = spot->size;

        // We may only move size in accordance with minUniformBufferOffsetAlignment
        const VkDeviceSize alignment = maxTwo(gRenderer->pd->props.limits.minStorageBufferOffsetAlignment, gRenderer->pd->props.limits.minUniformBufferOffsetAlignment);
        if (size % alignment != 0) {
            spot->size += ((size / alignment) + 1) * alignment;
        } else {
            spot->size += size;
        }
    }
}

void vk2dDescriptorBufferEndFrame(VK2DDescriptorBuffer db, VkCommandBuffer copyBuffer) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (vk2dStatusFatal() || gRenderer == NULL)
        return;

	// Unmap all of the buffers then queue a buffer copy if their size is greater than 0
	for (int i = 0; i < db->bufferCount; i++) {
		vmaUnmapMemory(gRenderer->vma, db->buffers[i].stageBuffer->mem);
		if (db->buffers[i].size > 0) {
			VkBufferCopy bufferCopy = {0};
			bufferCopy.size = (db->buffers[i].size < db->pageSize) ? db->buffers[i].size : db->pageSize;
			vkCmdCopyBuffer(copyBuffer, db->buffers[i].stageBuffer->buf, db->buffers[i].deviceBuffer->buf, 1, &bufferCopy);
		}
	}
}

void vk2dDescriptorBufferRecordCopyPipelineBarrier(VK2DDescriptorBuffer db, VkCommandBuffer buf) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal() || gRenderer == NULL)
        return;

    int barrierCount = 0;
    for (int i = 0; i < db->bufferCount; i++) {
        if (db->buffers[i].size > 0) {
            db->memoryBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            db->memoryBarriers[i].pNext = VK_NULL_HANDLE;
            db->memoryBarriers[i].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            db->memoryBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            db->memoryBarriers[i].srcQueueFamilyIndex = gRenderer->pd->QueueFamily.graphicsFamily;
            db->memoryBarriers[i].dstQueueFamilyIndex = gRenderer->pd->QueueFamily.graphicsFamily;
            db->memoryBarriers[i].buffer = db->buffers[i].deviceBuffer->buf;
            db->memoryBarriers[i].offset = 0;
            db->memoryBarriers[i].size = db->buffers[i].size;
            barrierCount++;
        } else {
            break;
        }
    }

    vkCmdPipelineBarrier(
            buf,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            0,
            VK_NULL_HANDLE,
            barrierCount,
            db->memoryBarriers,
            0,
            VK_NULL_HANDLE);
}

void vk2dDescriptorBufferRecordComputePipelineBarrier(VK2DDescriptorBuffer db, VkCommandBuffer buf) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal() || gRenderer == NULL)
        return;

    int barrierCount = 0;
    for (int i = 0; i < db->bufferCount; i++) {
        if (db->buffers[i].size > 0) {
            db->memoryBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            db->memoryBarriers[i].pNext = VK_NULL_HANDLE;
            db->memoryBarriers[i].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            db->memoryBarriers[i].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            db->memoryBarriers[i].srcQueueFamilyIndex = gRenderer->pd->QueueFamily.graphicsFamily;
            db->memoryBarriers[i].dstQueueFamilyIndex = gRenderer->pd->QueueFamily.graphicsFamily;
            db->memoryBarriers[i].buffer = db->buffers[i].deviceBuffer->buf;
            db->memoryBarriers[i].offset = 0;
            db->memoryBarriers[i].size = db->buffers[i].size;
            barrierCount++;
        } else {
            break;
        }
    }

    vkCmdPipelineBarrier(
            buf,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            0,
            0,
            VK_NULL_HANDLE,
            barrierCount,
            db->memoryBarriers,
            0,
            VK_NULL_HANDLE);
}
