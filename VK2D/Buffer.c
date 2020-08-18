/// \file Buffer.c
/// \author Paolo Mazzon
#include "VK2D/Buffer.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/PhysicalDevice.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include <malloc.h>

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

void vk2dBufferCopy(VK2DBuffer src, VK2DBuffer dst, uint32_t pool) {
	VkCommandBuffer buffer = vk2dLogicalDeviceGetSingleUseBuffer(src->dev, pool);
	VkBufferCopy copyRegion = {};
	copyRegion.size = src->size;
	copyRegion.dstOffset = 0;
	copyRegion.srcOffset = 0;
	vkCmdCopyBuffer(buffer, src->buf, dst->buf, 1, &copyRegion);
	vk2dLogicalDeviceSubmitSingleBuffer(src->dev, buffer, pool);
}

void vk2dBufferFree(VK2DBuffer buf) {
	if (buf != NULL) {
		vkFreeMemory(buf->dev->dev, buf->mem, VK_NULL_HANDLE);
		vkDestroyBuffer(buf->dev->dev, buf->buf, VK_NULL_HANDLE);
		free(buf);
	}
}