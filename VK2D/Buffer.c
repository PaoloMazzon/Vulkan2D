/// \file Buffer.c
/// \author Paolo Mazzon
#include "VK2D/Buffer.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/PhysicalDevice.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include <malloc.h>
#include <memory.h>

uint32_t _reVulkanBufferFindMemoryType(VkPhysicalDeviceMemoryProperties *memProps, uint32_t memoryFilter, VkMemoryPropertyFlags requirements) {
	uint32_t i;
	for (i = 0; i < memProps->memoryTypeCount; i++)
		if (memoryFilter & (1 << i) && (memProps->memoryTypes[i].propertyFlags & requirements) == requirements)
			return i;
	vk2dErrorCheck(-1);
	return UINT32_MAX;
}

VK2DBuffer vk2dBufferCreate(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem, VK2DLogicalDevice dev) {
	VK2DBuffer buf = malloc(sizeof(struct VK2DBuffer));

	if (vk2dPointerCheck(buf)) {
		buf->dev = dev;
		buf->size = size;
		VkBufferCreateInfo bufferCreateInfo = vk2dInitBufferCreateInfo(size, usage, &dev->pd->QueueFamily.graphicsFamily, 1);
		vk2dErrorCheck(vkCreateBuffer(dev->dev, &bufferCreateInfo, VK_NULL_HANDLE, &buf->buf));

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(dev->dev, buf->buf, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocateInfo = vk2dInitMemoryAllocateInfo(memoryRequirements.size, _reVulkanBufferFindMemoryType(&dev->pd->mem, memoryRequirements.memoryTypeBits, mem));
		vk2dErrorCheck(vkAllocateMemory(dev->dev, &memoryAllocateInfo, VK_NULL_HANDLE, &buf->mem));
		vkBindBufferMemory(dev->dev, buf->buf, buf->mem, 0);
	}

	return buf;
}

VK2DBuffer vk2dBufferLoad(VkDeviceSize size, VkBufferUsageFlags usage, VK2DLogicalDevice dev, void *data) {
	// Create staging buffer
	VK2DBuffer stageBuffer = vk2dBufferCreate(size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			dev);

	// Map data
	void *location;
	vk2dErrorCheck(vkMapMemory(dev->dev, stageBuffer->mem, 0, size, 0, &location));
	memcpy(location, data, size);
	vkUnmapMemory(dev->dev, stageBuffer->mem);

	// Create the actual vbo
	VK2DBuffer ret = vk2dBufferCreate(size,
			usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			dev);
	vk2dBufferCopy(stageBuffer, ret);

	vk2dBufferFree(stageBuffer);

	return ret;
}

void vk2dBufferCopy(VK2DBuffer src, VK2DBuffer dst) {
	VkCommandBuffer buffer = vk2dLogicalDeviceGetSingleUseBuffer(src->dev);
	VkBufferCopy copyRegion = {};
	copyRegion.size = src->size;
	copyRegion.dstOffset = 0;
	copyRegion.srcOffset = 0;
	vkCmdCopyBuffer(buffer, src->buf, dst->buf, 1, &copyRegion);
	vk2dLogicalDeviceSubmitSingleBuffer(src->dev, buffer);
}

void vk2dBufferFree(VK2DBuffer buf) {
	if (buf != NULL) {
		vkFreeMemory(buf->dev->dev, buf->mem, VK_NULL_HANDLE);
		vkDestroyBuffer(buf->dev->dev, buf->buf, VK_NULL_HANDLE);
		free(buf);
	}
}