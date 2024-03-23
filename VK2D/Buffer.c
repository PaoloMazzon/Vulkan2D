/// \file Buffer.c
/// \author Paolo Mazzon
#include "VK2D/Buffer.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/PhysicalDevice.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include <malloc.h>
#include <memory.h>
#include "VK2D/Renderer.h"
#include "VK2D/Opaque.h"

uint32_t _reVulkanBufferFindMemoryType(VkPhysicalDeviceMemoryProperties *memProps, uint32_t memoryFilter, VkMemoryPropertyFlags requirements) {
	uint32_t i;
	for (i = 0; i < memProps->memoryTypeCount; i++)
		if (memoryFilter & (1 << i) && (memProps->memoryTypes[i].propertyFlags & requirements) == requirements)
			return i;
	vk2dErrorCheck(-1);
	return UINT32_MAX;
}

VK2DBuffer vk2dBufferCreate(VK2DLogicalDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	VK2DBuffer buf = malloc(sizeof(struct VK2DBuffer_t));

	if (vk2dPointerCheck(buf)) {
		buf->dev = dev;
		buf->size = size;
		buf->offset = 0;
		VkBufferCreateInfo bufferCreateInfo = vk2dInitBufferCreateInfo(size, usage, &dev->pd->QueueFamily.graphicsFamily, 1);
		VmaAllocationCreateInfo allocationCreateInfo = {0};
		allocationCreateInfo.requiredFlags = mem;
		vk2dErrorCheck(vmaCreateBuffer(gRenderer->vma, &bufferCreateInfo, &allocationCreateInfo, &buf->buf, &buf->mem, VK_NULL_HANDLE));
	}

	return buf;
}

VK2DBuffer vk2dBufferLoad(VK2DLogicalDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, void *data, bool mainThread) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	// Create staging buffer
	VK2DBuffer stageBuffer = vk2dBufferCreate(dev,
			size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Map data
	void *location;
	vk2dErrorCheck(vmaMapMemory(gRenderer->vma, stageBuffer->mem, &location));
	memcpy(location, data, size);
	vmaUnmapMemory(gRenderer->vma, stageBuffer->mem);

	// Create the actual vbo
	VK2DBuffer ret = vk2dBufferCreate(dev,
			size,
			usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vk2dBufferCopy(stageBuffer, ret, mainThread);

	vk2dBufferFree(stageBuffer);

	return ret;
}

VK2DBuffer vk2dBufferLoad2(VK2DLogicalDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, void *data, VkDeviceSize size2, void *data2, bool mainThread) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	// Create staging buffer
	VK2DBuffer stageBuffer = vk2dBufferCreate(dev,
											  size + size2,
											  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
											  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Map data
	void *location;
	vk2dErrorCheck(vmaMapMemory(gRenderer->vma, stageBuffer->mem, &location));
	memcpy(location, data, size);
	uint8_t *ll = location;
	memcpy(ll + size, data2, size2);
	vmaUnmapMemory(gRenderer->vma, stageBuffer->mem);

	// Create the buffer
	VK2DBuffer ret = vk2dBufferCreate(dev,
									  size + size2,
									  usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
									  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vk2dBufferCopy(stageBuffer, ret, mainThread);

	vk2dBufferFree(stageBuffer);

	return ret;
}

void vk2dBufferCopy(VK2DBuffer src, VK2DBuffer dst, bool mainThread) {
	VkCommandBuffer buffer = vk2dLogicalDeviceGetSingleUseBuffer(src->dev, mainThread);
	VkBufferCopy copyRegion = {0};
	copyRegion.size = src->size;
	copyRegion.dstOffset = 0;
	copyRegion.srcOffset = 0;
	vkCmdCopyBuffer(buffer, src->buf, dst->buf, 1, &copyRegion);
	vk2dLogicalDeviceSubmitSingleBuffer(src->dev, buffer, mainThread);
}

void vk2dBufferFree(VK2DBuffer buf) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (buf != NULL) {
		vmaDestroyBuffer(gRenderer->vma, buf->buf, buf->mem);
		free(buf);
	}
}