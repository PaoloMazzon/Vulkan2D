/// \file Buffer.c
/// \author Paolo Mazzon
#include "VK2D/Buffer.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include <malloc.h>
#include "VK2D/Renderer.h"
#include "VK2D/Opaque.h"

VK2DBuffer vk2dBufferCreate(VK2DLogicalDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer == NULL || vk2dStatusFatal())
	    return NULL;

	VK2DBuffer buf = malloc(sizeof(struct VK2DBuffer_t));

	if (buf != NULL) {
		buf->dev = dev;
		buf->size = size;
		buf->offset = 0;
		VkBufferCreateInfo bufferCreateInfo = vk2dInitBufferCreateInfo(size, usage, &dev->pd->QueueFamily.graphicsFamily, 1);
		VmaAllocationCreateInfo allocationCreateInfo = {0};
		allocationCreateInfo.requiredFlags = mem;
		VkResult result = vmaCreateBuffer(gRenderer->vma, &bufferCreateInfo, &allocationCreateInfo, &buf->buf, &buf->mem, VK_NULL_HANDLE);
		if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            vk2dRaise(VK2D_STATUS_OUT_OF_VRAM, "VMA out of video memory for buffer of size %0.2fkb.", (float)size / 1024.0f);
            free(buf);
            buf = NULL;
		} else if (result != VK_SUCCESS) {
            vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to allocate buffer of size %0.2fkb.", (float)size / 1024.0f);
            free(buf);
            buf = NULL;
		}
	} else {
	    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate buffer struct.");
	}

	return buf;
}

VK2DBuffer vk2dBufferLoad(VK2DLogicalDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, void *data, bool mainThread) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (gRenderer == NULL || vk2dStatusFatal())
        return NULL;

	// Create staging buffer
	VK2DBuffer stageBuffer = vk2dBufferCreate(dev,
			size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (stageBuffer == NULL)
	    return NULL;

	// Map data
	void *location;
	VkResult result = vmaMapMemory(gRenderer->vma, stageBuffer->mem, &location);
	if (result != VK_SUCCESS) {
	    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to map memory, VMA error %i.", result);
	    vk2dBufferFree(stageBuffer);
	    return NULL;
	}
	memcpy(location, data, size);
	vmaUnmapMemory(gRenderer->vma, stageBuffer->mem);

	// Create the actual vbo
	VK2DBuffer ret = vk2dBufferCreate(dev,
			size,
			usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (ret != NULL)
	    vk2dBufferCopy(stageBuffer, ret, mainThread);
	vk2dBufferFree(stageBuffer);

	return ret;
}

VK2DBuffer vk2dBufferLoad2(VK2DLogicalDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, void *data, VkDeviceSize size2, void *data2, bool mainThread) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (gRenderer == NULL || vk2dStatusFatal())
        return NULL;

	// Create staging buffer
	VK2DBuffer stageBuffer = vk2dBufferCreate(dev,
											  size + size2,
											  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
											  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (stageBuffer == NULL)
	    return NULL;

	// Map data
	void *location;
	VkResult result = vmaMapMemory(gRenderer->vma, stageBuffer->mem, &location);
    if (result != VK_SUCCESS) {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to map memory, VMA error %i.", result);
        vk2dBufferFree(stageBuffer);
        return NULL;
    }
	memcpy(location, data, size);
	uint8_t *ll = location;
	memcpy(ll + size, data2, size2);
	vmaUnmapMemory(gRenderer->vma, stageBuffer->mem);

	// Create the buffer
	VK2DBuffer ret = vk2dBufferCreate(dev,
									  size + size2,
									  usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
									  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (ret != NULL)
	    vk2dBufferCopy(stageBuffer, ret, mainThread);

	vk2dBufferFree(stageBuffer);

	return ret;
}

void vk2dBufferCopy(VK2DBuffer src, VK2DBuffer dst, bool mainThread) {
    if (vk2dRendererGetPointer() == NULL || vk2dStatusFatal())
        return;
	VkCommandBuffer buffer = vk2dLogicalDeviceGetSingleUseBuffer(src->dev, mainThread);
	if (buffer != VK_NULL_HANDLE) {
        VkBufferCopy copyRegion = {0};
        copyRegion.size = src->size;
        copyRegion.dstOffset = 0;
        copyRegion.srcOffset = 0;
        vkCmdCopyBuffer(buffer, src->buf, dst->buf, 1, &copyRegion);
        vk2dLogicalDeviceSubmitSingleBuffer(src->dev, buffer, mainThread);
    }
}

void vk2dBufferFree(VK2DBuffer buf) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (gRenderer == NULL || vk2dStatusFatal())
        return;
	if (buf != NULL) {
		vmaDestroyBuffer(gRenderer->vma, buf->buf, buf->mem);
		free(buf);
	}
}