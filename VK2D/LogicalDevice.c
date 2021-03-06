/// \file LogicalDevice.c
/// \author Paolo Mazzon
#include "VK2D/LogicalDevice.h"
#include "VK2D/Initializers.h"
#include "VK2D/Validation.h"
#include "VK2D/PhysicalDevice.h"
#include <malloc.h>

VK2DLogicalDevice vk2dLogicalDeviceCreate(VK2DPhysicalDevice dev, bool enableAllFeatures, bool graphicsDevice) {
	VK2DLogicalDevice ldev = malloc(sizeof(struct VK2DLogicalDevice));
	uint32_t queueFamily = graphicsDevice == true ? dev->QueueFamily.graphicsFamily : dev->QueueFamily.computeFamily;

	if (vk2dPointerCheck(ldev)) {
		// Assemble the required features
		VkPhysicalDeviceFeatures feats = {};
		if (enableAllFeatures) {
			feats = dev->feats;
		} else {
			feats.wideLines = VK_TRUE;
			feats.fillModeNonSolid = VK_TRUE;
			feats.samplerAnisotropy = VK_TRUE;
			feats.shaderStorageImageMultisample = VK_TRUE;
		}

		float priority = 1;
		VkDeviceQueueCreateInfo queueCreateInfo = vk2dInitDeviceQueueCreateInfo(queueFamily, &priority);
		VkDeviceCreateInfo deviceCreateInfo = vk2dInitDeviceCreateInfo(&queueCreateInfo, 1, &feats);
		VkCommandPoolCreateInfo commandPoolCreateInfo = vk2dInitCommandPoolCreateInfo(queueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		vk2dErrorCheck(vkCreateDevice(dev->dev, &deviceCreateInfo, VK_NULL_HANDLE, &ldev->dev));
		ldev->pd = dev;
		vkGetDeviceQueue(ldev->dev, queueFamily, 0, &ldev->queue);

		vk2dErrorCheck(vkCreateCommandPool(ldev->dev, &commandPoolCreateInfo, VK_NULL_HANDLE, &ldev->pool));
	}
	
	return ldev;
}

void vk2dLogicalDeviceFree(VK2DLogicalDevice dev) {
	if (dev != NULL) {
		vkDestroyCommandPool(dev->dev, dev->pool, VK_NULL_HANDLE);
		vkDestroyDevice(dev->dev, VK_NULL_HANDLE);
		free(dev);
	}
}

void vk2dLogicalDeviceResetPool(VK2DLogicalDevice dev) {
	vk2dErrorCheck(vkResetCommandPool(dev->dev, dev->pool, 0));
}

VkCommandBuffer vk2dLogicalDeviceGetCommandBuffer(VK2DLogicalDevice dev, bool primary) {
	VkCommandBufferAllocateInfo allocInfo = vk2dInitCommandBufferAllocateInfo(dev->pool, 1);
	allocInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	VkCommandBuffer buffer;
	vk2dErrorCheck(vkAllocateCommandBuffers(dev->dev, &allocInfo, &buffer));
	return buffer;
}

void vk2dLogicalDeviceGetCommandBuffers(VK2DLogicalDevice dev, bool primary, uint32_t n, VkCommandBuffer *list) {
	VkCommandBufferAllocateInfo allocInfo = vk2dInitCommandBufferAllocateInfo(dev->pool, n);
	allocInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	vk2dErrorCheck(vkAllocateCommandBuffers(dev->dev, &allocInfo, list));
}

void vk2dLogicalDeviceFreeCommandBuffer(VK2DLogicalDevice dev, VkCommandBuffer buffer) {
	vkFreeCommandBuffers(dev->dev, dev->pool, 1, &buffer);
}

VkCommandBuffer vk2dLogicalDeviceGetSingleUseBuffer(VK2DLogicalDevice dev) {
	VkCommandBufferAllocateInfo allocInfo = vk2dInitCommandBufferAllocateInfo(dev->pool, 1);
	VkCommandBuffer buffer;
	vk2dErrorCheck(vkAllocateCommandBuffers(dev->dev, &allocInfo, &buffer));
	VkCommandBufferBeginInfo beginInfo = vk2dInitCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);
	vk2dErrorCheck(vkBeginCommandBuffer(buffer, &beginInfo));
	return buffer;
}

void vk2dLogicalDeviceSubmitSingleBuffer(VK2DLogicalDevice dev, VkCommandBuffer buffer) {
	VkSubmitInfo submitInfo = vk2dInitSubmitInfo(&buffer, 1, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE);
	vkEndCommandBuffer(buffer);
	vkQueueSubmit(dev->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(dev->queue);
	vkFreeCommandBuffers(dev->dev, dev->pool, 1, &buffer);
}

VkFence vk2dLogicalDeviceGetFence(VK2DLogicalDevice dev, VkFenceCreateFlagBits flags) {
	VkFenceCreateInfo fenceCreateInfo = vk2dInitFenceCreateInfo(flags);
	VkFence fence;
	vk2dErrorCheck(vkCreateFence(dev->dev, &fenceCreateInfo, VK_NULL_HANDLE, &fence));
	return fence;
}

void vk2dLogicalDeviceFreeFence(VK2DLogicalDevice dev, VkFence fence) {
	vkDestroyFence(dev->dev, fence, VK_NULL_HANDLE);
}

VkSemaphore vk2dLogicalDeviceGetSemaphore(VK2DLogicalDevice dev) {
	VkSemaphoreCreateInfo semaphoreCreateInfo = vk2dInitSemaphoreCreateInfo(0);
	VkSemaphore semaphore;
	vk2dErrorCheck(vkCreateSemaphore(dev->dev, &semaphoreCreateInfo, VK_NULL_HANDLE, &semaphore));
	return semaphore;
}

void vk2dLogicalDeviceFreeSemaphore(VK2DLogicalDevice dev, VkSemaphore semaphore) {
	vkDestroySemaphore(dev->dev, semaphore, VK_NULL_HANDLE);
}