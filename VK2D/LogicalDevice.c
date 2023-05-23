/// \file LogicalDevice.c
/// \author Paolo Mazzon
#include "VK2D/LogicalDevice.h"
#include "VK2D/Initializers.h"
#include "VK2D/Validation.h"
#include "VK2D/PhysicalDevice.h"
#include "VK2D/Opaque.h"
#include "VK2D/Util.h"
#include "VK2D/Renderer.h"
#include <malloc.h>

VK2DLogicalDevice gDeviceFromMainThread;

VK2DLogicalDevice vk2dLogicalDeviceCreate(VK2DPhysicalDevice dev, bool enableAllFeatures, bool graphicsDevice, bool debug, VK2DRendererLimits *limits) {
	vk2dLogMessage("Creating queues...");
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	VK2DLogicalDevice ldev = malloc(sizeof(struct VK2DLogicalDevice_t));
	uint32_t queueFamily = graphicsDevice == true ? dev->QueueFamily.graphicsFamily : dev->QueueFamily.computeFamily;

	if (vk2dPointerCheck(ldev)) {
		// Assemble the required features
		VkPhysicalDeviceFeatures feats = {0};
		if (enableAllFeatures) {
			feats = dev->feats;
		} else {
			if (dev->feats.wideLines) {
				feats.wideLines = VK_TRUE;
				limits->maxLineWidth = dev->props.limits.lineWidthRange[1];
			} else {
				limits->maxLineWidth = 1;
			}
			if (dev->feats.fillModeNonSolid) {
				feats.fillModeNonSolid = VK_TRUE;
				limits->supportsWireframe = true;
			} else {
				limits->supportsWireframe = false;
			}
			if (dev->feats.samplerAnisotropy) {
				feats.samplerAnisotropy = VK_TRUE;
				limits->maxMSAA = vk2dPhysicalDeviceGetMSAA(dev);
			} else {
				limits->maxMSAA = 1;
			}
		}

		float priority[] = {1, 1};
		VkDeviceQueueCreateInfo queueCreateInfo = vk2dInitDeviceQueueCreateInfo(queueFamily, priority);
		queueCreateInfo.queueCount = gRenderer->limits.supportsMultiThreadLoading ? 2 : 1;
		VkDeviceQueueCreateInfo queues[] = {queueCreateInfo};
		VkDeviceCreateInfo deviceCreateInfo = vk2dInitDeviceCreateInfo(queues, 1, &feats, debug);
		vk2dErrorCheck(vkCreateDevice(dev->dev, &deviceCreateInfo, VK_NULL_HANDLE, &ldev->dev));
		ldev->pd = dev;
		vkGetDeviceQueue(ldev->dev, queueFamily, 0, &ldev->queue);
		if (queueCreateInfo.queueCount == 2)
			vkGetDeviceQueue(ldev->dev, queueFamily, 1, &ldev->loadQueue);

		VkCommandPoolCreateInfo commandPoolCreateInfo = vk2dInitCommandPoolCreateInfo(queueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		vk2dErrorCheck(vkCreateCommandPool(ldev->dev, &commandPoolCreateInfo, VK_NULL_HANDLE, &ldev->pool));

		if (gRenderer->limits.supportsMultiThreadLoading) {
			vk2dLogMessage("Creating worker thread...");
			ldev->loadList = NULL;
			ldev->loadListMutex = SDL_CreateMutex();
			ldev->shaderMutex = SDL_CreateMutex();
			ldev->loadListSize = 0;
			ldev->quitThread = false;
			ldev->loads = 0;
			gDeviceFromMainThread = ldev;
			ldev->workerThread = SDL_CreateThread(_vk2dWorkerThread, "VK2D_Load", NULL);

			if (ldev->loadListMutex == NULL || ldev->workerThread == NULL || ldev->shaderMutex == NULL) {
				vk2dLogMessage("Failed to initialize off-thread loading: %s", SDL_GetError());
			}
		}
	}
	
	return ldev;
}

void vk2dLogicalDeviceFree(VK2DLogicalDevice dev) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (dev != NULL) {
		dev->quitThread = true;
		int status;
		if (gRenderer->limits.supportsMultiThreadLoading) {
			SDL_WaitThread(dev->workerThread, &status);
			SDL_DestroyMutex(dev->loadListMutex);
			SDL_DestroyMutex(dev->shaderMutex);
			vkDestroyCommandPool(dev->dev, dev->loadPool, VK_NULL_HANDLE);
		}
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

VkCommandBuffer vk2dLogicalDeviceGetSingleUseBuffer(VK2DLogicalDevice dev, bool mainThread) {
	VkCommandBufferAllocateInfo allocInfo = vk2dInitCommandBufferAllocateInfo(dev->pool, 1);
	if (!mainThread)
		allocInfo.commandPool = dev->loadPool;
	VkCommandBuffer buffer;
	vk2dErrorCheck(vkAllocateCommandBuffers(dev->dev, &allocInfo, &buffer));
	VkCommandBufferBeginInfo beginInfo = vk2dInitCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);
	vk2dErrorCheck(vkBeginCommandBuffer(buffer, &beginInfo));
	return buffer;
}

void vk2dLogicalDeviceSubmitSingleBuffer(VK2DLogicalDevice dev, VkCommandBuffer buffer, bool mainThread) {
	VkSubmitInfo submitInfo = vk2dInitSubmitInfo(&buffer, 1, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE);
	vkEndCommandBuffer(buffer);
	if (mainThread) {
		vkQueueSubmit(dev->queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(dev->queue);
		vkFreeCommandBuffers(dev->dev, dev->pool, 1, &buffer);
	} else {
		vkQueueSubmit(dev->loadQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(dev->loadQueue);
		vkFreeCommandBuffers(dev->dev, dev->loadPool, 1, &buffer);
	}
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