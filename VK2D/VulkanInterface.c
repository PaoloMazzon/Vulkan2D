/// \file VulkanInterface.c
/// \author Paolo Mazzon
#include "VK2D/VulkanInterface.h"
#include "VK2D/Opaque.h"
#include "VK2D/Renderer.h"
#include "VK2D/RendererMeta.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/DescriptorBuffer.h"

VkCommandBuffer vk2dVulkanGetSingleUseBuffer() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return vk2dLogicalDeviceGetSingleUseBuffer(gRenderer->ld, true);
}

void vk2dVulkanSubmitSingleUseBuffer(VkCommandBuffer buffer) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vk2dLogicalDeviceSubmitSingleBuffer(gRenderer->ld, buffer, true);
}

VkCommandBuffer vk2dVulkanGetDrawBuffer() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	_vk2dRendererResetBoundPointers();
	return gRenderer->commandBuffer[gRenderer->scImageIndex];
}

void vk2dVulkanCopyDataIntoBuffer(void *data, VkDeviceSize size, VkBuffer *outBuffer, VkDeviceSize *bufferOffset) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vk2dDescriptorBufferCopyData(gRenderer->descriptorBuffers[gRenderer->currentFrame], data, size, outBuffer, bufferOffset);
}

int vk2dVulkanGetFrame() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return gRenderer->currentFrame;
}

int vk2dVulkanGetSwapchainImageIndex() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return gRenderer->scImageIndex;
}

VkDevice vk2dVulkanGetDevice() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return gRenderer->ld->dev;
}

VkPhysicalDevice vk2dVulkanGetPhysicalDevice() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return gRenderer->pd->dev;
}

VkQueue vk2dVulkanGetQueue() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return gRenderer->ld->queue;
}

uint32_t vk2dVulkanGetQueueFamily() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return gRenderer->pd->QueueFamily.graphicsFamily;
}

uint32_t vk2dVulkanGetSwapchainImageCount() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return gRenderer->swapchainImageCount;
}

uint32_t vk2dVulkanGetMaxFramesInFlight() {
	return VK2D_MAX_FRAMES_IN_FLIGHT;
}

VmaAllocator vk2dVulkanGetVMA() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return gRenderer->vma;
}
