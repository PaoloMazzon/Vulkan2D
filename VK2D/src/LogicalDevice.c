/// \file LogicalDevice.c
/// \author Paolo Mazzon
#include "VK2D/LogicalDevice.h"
#include "VK2D/Initializers.h"
#include "VK2D/Validation.h"
#include "VK2D/PhysicalDevice.h"
#include "VK2D/Opaque.h"
#include "VK2D/Util.h"
#include "VK2D/Renderer.h"
#include "VK2D/Logger.h"
#include <stdlib.h>

VK2DLogicalDevice gDeviceFromMainThread;

VK2DLogicalDevice vk2dLogicalDeviceCreate(VK2DPhysicalDevice dev, bool enableAllFeatures, bool graphicsDevice, bool debug, VK2DRendererLimits *limits) {
    vk2dLogInfo("Creating queues...");
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	VK2DLogicalDevice ldev = malloc(sizeof(struct VK2DLogicalDevice_t));
	uint32_t queueFamily = graphicsDevice == true ? dev->QueueFamily.graphicsFamily : dev->QueueFamily.computeFamily;

	// Find any optional extensions
	uint32_t extensionCount = 0;
	VkExtensionProperties *props = VK_NULL_HANDLE;
	vkEnumerateDeviceExtensionProperties(dev->dev, VK_NULL_HANDLE, &extensionCount, VK_NULL_HANDLE);
	props = malloc(extensionCount * sizeof(struct VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(dev->dev, VK_NULL_HANDLE, &extensionCount, props);
    const bool instanceExtensionSupported = gRenderer->limits.supportsVRAMUsage;
    gRenderer->limits.supportsVRAMUsage = false;
    bool portabilitySubset = false;
	for (int i = 0; i < extensionCount; i++) {
	    if (strcmp(props[i].extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0 && instanceExtensionSupported)
	        gRenderer->limits.supportsVRAMUsage = true;
	    portabilitySubset |= strcmp(props[i].extensionName, "VK_KHR_portability_subset") == 0;
	}
    free(props);

	// Find limits
	if (ldev != NULL) {
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

		// For dynamic descriptor arrays
        VkPhysicalDeviceDescriptorIndexingFeatures supported = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};
        VkPhysicalDeviceFeatures2 feats2 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &supported};
        vkGetPhysicalDeviceFeatures2(dev->dev, &feats2);
        const struct {const char *name; VkBool32 have;} required[] = {
                {"shaderSampledImageArrayNonUniformIndexing", supported.shaderSampledImageArrayNonUniformIndexing},
                {"runtimeDescriptorArray", supported.runtimeDescriptorArray},
                {"descriptorBindingVariableDescriptorCount", supported.descriptorBindingVariableDescriptorCount},
                {"descriptorBindingPartiallyBound", supported.descriptorBindingPartiallyBound},
                {"descriptorBindingUpdateUnusedWhilePending", supported.descriptorBindingUpdateUnusedWhilePending},
                {"descriptorBindingSampledImageUpdateAfterBind", supported.descriptorBindingSampledImageUpdateAfterBind},
                {"descriptorBindingUniformBufferUpdateAfterBind", supported.descriptorBindingUniformBufferUpdateAfterBind},
        };
        for (int i = 0; i < (int)(sizeof(required) / sizeof(required[0])); i++) {
            if (!required[i].have) {
                vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Device does not support required descriptor indexing feature %s.", required[i].name);
                free(ldev);
                return NULL;
            }
        }
        VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
                .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
                .runtimeDescriptorArray = VK_TRUE,
                .descriptorBindingVariableDescriptorCount = VK_TRUE,
                .descriptorBindingPartiallyBound = VK_TRUE,
                .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
                .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
				.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
		};

        // Clamp the texture array to what the device can bind
        VkPhysicalDeviceDescriptorIndexingProperties indexingProps = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES};
        VkPhysicalDeviceProperties2 props2 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &indexingProps};
        vkGetPhysicalDeviceProperties2(dev->dev, &props2);
        uint32_t maxTextures = indexingProps.maxDescriptorSetUpdateAfterBindSampledImages;
        if (indexingProps.maxPerStageDescriptorUpdateAfterBindSampledImages < maxTextures)
            maxTextures = indexingProps.maxPerStageDescriptorUpdateAfterBindSampledImages;
        if (gRenderer->options.maxTextures > maxTextures) {
            vk2dLogInfo("Clamping max textures from %u to device limit %u.", gRenderer->options.maxTextures, maxTextures);
            gRenderer->options.maxTextures = maxTextures;
        }
        vk2dLogInfo("Texture descriptor limit: %u", gRenderer->options.maxTextures);

		// Basic device create info
		float priority[] = {1, 1};
		VkDeviceQueueCreateInfo queueCreateInfo = vk2dInitDeviceQueueCreateInfo(queueFamily, priority);
		queueCreateInfo.queueCount = gRenderer->limits.supportsMultiThreadLoading ? 2 : 1;
		VkDeviceQueueCreateInfo queues[] = {queueCreateInfo};
		VkDeviceCreateInfo deviceCreateInfo = vk2dInitDeviceCreateInfo(queues, 1, &feats, debug);
        deviceCreateInfo.pNext = &indexingFeatures;

        // Device layers and extensions
        const char *deviceExtensions[10] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        int deviceExtensionCount = 1;
        if (gRenderer->limits.supportsVRAMUsage) {
            deviceExtensions[deviceExtensionCount++] = VK_EXT_MEMORY_BUDGET_EXTENSION_NAME;
        }
        if (portabilitySubset) {
            deviceExtensions[deviceExtensionCount++] = "VK_KHR_portability_subset";
        }
        deviceCreateInfo.enabledExtensionCount = deviceExtensionCount;
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = VK_NULL_HANDLE;

        vk2dLogInfo("Vulkan Enabled Device Extensions: ");
        for (int i = 0; i < deviceExtensionCount; i++)
            vk2dLogInfo(" - %s", deviceExtensions[i]);
        vk2dLogInfo("");

        // Create device
		VkResult result = vkCreateDevice(dev->dev, &deviceCreateInfo, VK_NULL_HANDLE, &ldev->dev);
		if (result != VK_SUCCESS) {
		    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create logical device, Vulkan error %i.", result);
		    free(ldev);
		    return NULL;
		}
		ldev->pd = dev;
		vkGetDeviceQueue(ldev->dev, queueFamily, 0, &ldev->queue);
		if (queueCreateInfo.queueCount == 2)
			vkGetDeviceQueue(ldev->dev, queueFamily, 1, &ldev->loadQueue);

		VkCommandPoolCreateInfo commandPoolCreateInfo = vk2dInitCommandPoolCreateInfo(queueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		result = vkCreateCommandPool(ldev->dev, &commandPoolCreateInfo, VK_NULL_HANDLE, &ldev->pool);
        if (result != VK_SUCCESS) {
            vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create command pool, Vulkan error %i.", result);
            free(ldev);
            return NULL;
        }
	} else {
	    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate logical device.");
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
	VkResult result = vkResetCommandPool(dev->dev, dev->pool, 0);
	if (result != VK_SUCCESS) {
	    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to reset command pool, Vulkan error %i", result);
	}
}

VkCommandBuffer vk2dLogicalDeviceGetCommandBuffer(VK2DLogicalDevice dev, bool primary) {
	VkCommandBufferAllocateInfo allocInfo = vk2dInitCommandBufferAllocateInfo(dev->pool, 1);
	allocInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	VkCommandBuffer buffer;
	VkResult result = vkAllocateCommandBuffers(dev->dev, &allocInfo, &buffer);
    if (result != VK_SUCCESS) {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to get command buffer, Vulkan error %i", result);
    }
	return buffer;
}

void vk2dLogicalDeviceGetCommandBuffers(VK2DLogicalDevice dev, bool primary, uint32_t n, VkCommandBuffer *list) {
	VkCommandBufferAllocateInfo allocInfo = vk2dInitCommandBufferAllocateInfo(dev->pool, n);
	allocInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	VkResult result = vkAllocateCommandBuffers(dev->dev, &allocInfo, list);
    if (result != VK_SUCCESS) {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to reset command pool, Vulkan error %i", result);
    }
}

void vk2dLogicalDeviceFreeCommandBuffer(VK2DLogicalDevice dev, VkCommandBuffer buffer) {
	vkFreeCommandBuffers(dev->dev, dev->pool, 1, &buffer);
}

VkCommandBuffer vk2dLogicalDeviceGetSingleUseBuffer(VK2DLogicalDevice dev) {
	VkCommandBufferAllocateInfo allocInfo = vk2dInitCommandBufferAllocateInfo(dev->pool, 1);
	VkCommandBuffer buffer;
	VkResult result = vkAllocateCommandBuffers(dev->dev, &allocInfo, &buffer);
    if (result != VK_SUCCESS) {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to allocate command buffers, Vulkan error %i", result);
    }
	VkCommandBufferBeginInfo beginInfo = vk2dInitCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);
	result = vkBeginCommandBuffer(buffer, &beginInfo);
    if (result != VK_SUCCESS) {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to begin command buffer, Vulkan error %i", result);
    }
	return buffer;
}

void vk2dLogicalDeviceSubmitSingleBuffer(VK2DLogicalDevice dev, VkCommandBuffer buffer) {
	VkSubmitInfo submitInfo = vk2dInitSubmitInfo(&buffer, 1, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE);
	vkEndCommandBuffer(buffer);
	VkResult result = vkQueueSubmit(dev->queue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result == VK_SUCCESS) {
        result = vkQueueWaitIdle(dev->queue);
        if (result == VK_SUCCESS) {
            vkFreeCommandBuffers(dev->dev, dev->pool, 1, &buffer);
        } else {
            vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to wait for queue, Vulkan error %i", result);
        }
    } else {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to submit queue, Vulkan error %i", result);
    }
}

VkFence vk2dLogicalDeviceGetFence(VK2DLogicalDevice dev, VkFenceCreateFlagBits flags) {
	VkFenceCreateInfo fenceCreateInfo = vk2dInitFenceCreateInfo(flags);
	VkFence fence;
	VkResult result = vkCreateFence(dev->dev, &fenceCreateInfo, VK_NULL_HANDLE, &fence);
	if (result != VK_SUCCESS) {
	    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create fence, Vulkan error %i", result);
	}
	return fence;
}

void vk2dLogicalDeviceFreeFence(VK2DLogicalDevice dev, VkFence fence) {
	vkDestroyFence(dev->dev, fence, VK_NULL_HANDLE);
}

VkSemaphore vk2dLogicalDeviceGetSemaphore(VK2DLogicalDevice dev) {
	VkSemaphoreCreateInfo semaphoreCreateInfo = vk2dInitSemaphoreCreateInfo(0);
	VkSemaphore semaphore;
	VkResult result = vkCreateSemaphore(dev->dev, &semaphoreCreateInfo, VK_NULL_HANDLE, &semaphore);
	if (result != VK_SUCCESS) {
	    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create semaphore, Vulkan error %i", result);
	}
	return semaphore;
}

void vk2dLogicalDeviceFreeSemaphore(VK2DLogicalDevice dev, VkSemaphore semaphore) {
	vkDestroySemaphore(dev->dev, semaphore, VK_NULL_HANDLE);
}