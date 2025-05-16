/// \file PhysicalDevice.c
/// \author Paolo Mazzon
#include "VK2D/PhysicalDevice.h"
#include "VK2D/Initializers.h"
#include "VK2D/Validation.h"
#include "VK2D/Constants.h"
#include "VK2D/Opaque.h"
#include "VK2D/Renderer.h"
#include "VK2D/Logger.h"
#include <stdio.h>
#include <malloc.h>

static VkPhysicalDevice* _vk2dPhysicalDeviceGetPhysicalDevices(VkInstance instance, uint32_t *size) {
	VkPhysicalDevice *devices;

	// Find out how many devices we have, allocate that amount, then fill in the blanks
	VkResult result = vkEnumeratePhysicalDevices(instance, size, NULL);
	if (result == VK_SUCCESS) {
        devices = malloc(sizeof(VkPhysicalDevice) * (*size));
        if (devices != NULL) {
            result = vkEnumeratePhysicalDevices(instance, size, devices);
            if (result != VK_SUCCESS) {
                free(devices);
                devices = NULL;
                vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to enumerate devices, Vulkan error %i.", result);
            }
        } else {
            vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate %i devices.", *size);
        }
    } else {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to enumerate devices, Vulkan error %i.", result);
	}

	return devices;
}

static bool _vk2dPhysicalDeviceSupportsQueueFamilies(VkInstance instance, VkPhysicalDevice dev, VK2DPhysicalDevice out) {
	uint32_t queueFamilyCount = 0;
	uint32_t i;
	VkQueueFamilyProperties* queueList;
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, NULL);
	queueList = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	bool gfx = false;
	bool comp = false;

	if (queueList != NULL) {
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueList);
        for (i = 0; i < queueFamilyCount && !gfx; i++) {
            if (queueList[i].queueCount > 0) {
                if (queueList[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && queueList[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    out->QueueFamily.graphicsFamily = i;
                    gRenderer->limits.supportsMultiThreadLoading = queueList[i].queueCount >= 2;
                    gfx = true;
                    out->QueueFamily.computeFamily = i;
                    comp = true;
                }
            }
        }
    } else {
	    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate queue family properties.");
	}

	free(queueList);
	return gfx && comp;
}

// Checks if a device is supported, loading all the queue families if so
static bool _vk2dPhysicalDeviceSupported(VkInstance instance, VkPhysicalDevice dev, VkPhysicalDeviceProperties *props, VK2DPhysicalDevice out) {
	bool supportsQueueFamily = _vk2dPhysicalDeviceSupportsQueueFamilies(instance, dev, out);
	return supportsQueueFamily;
}

VkPhysicalDeviceProperties *vk2dPhysicalDeviceGetList(VkInstance instance, uint32_t *size) {
	VkPhysicalDevice *devs = _vk2dPhysicalDeviceGetPhysicalDevices(instance, size);
	VkPhysicalDeviceProperties *props = NULL;

	if (devs != NULL) {
		props = malloc(sizeof(VkPhysicalDeviceProperties) * (*size));
		if (props != NULL) {
			uint32_t i;
			for (i = 0; i < *size; i++)
				vkGetPhysicalDeviceProperties(devs[i], &props[i]);
		} else {
		    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate device properties.");
		}
	}

	free(devs);
	return props;
}

static VkPhysicalDevice _vk2dPhysicalDeviceGetBestDevice(VkInstance instance, VK2DPhysicalDevice out, int32_t preferredDevice, bool *foundPrimary) {
	uint32_t devCount;
	VkPhysicalDevice choice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties choiceProps;
	*foundPrimary = false;
	VkPhysicalDevice *devs = _vk2dPhysicalDeviceGetPhysicalDevices(instance, &devCount);
	if (devs != NULL && preferredDevice == VK2D_DEVICE_BEST_FIT) {
		uint32_t i;
		for (i = 0; i < devCount && !(*foundPrimary); i++) {
			vkGetPhysicalDeviceProperties(devs[i], &choiceProps);
			if (_vk2dPhysicalDeviceSupported(instance, devs[i], &choiceProps, out)) {
				if (choiceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
					*foundPrimary = true;
				choice = devs[i];
			}
		}
	}

	// Use preferred device after checking its valid
	if (preferredDevice != VK2D_DEVICE_BEST_FIT) {
		if (preferredDevice < devCount && devCount >= 0) {
			choice = devs[preferredDevice];
			vkGetPhysicalDeviceProperties(choice, &choiceProps);
		} else {
			vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Device \"%i\" out of range.", preferredDevice);
			free(out);
			out = NULL;
		}
	}

	free(devs);
	return choice;
}

VK2DPhysicalDevice vk2dPhysicalDeviceFind(VkInstance instance, int32_t preferredDevice) {
	VK2DPhysicalDevice out = malloc(sizeof(struct VK2DPhysicalDevice_t));
	VkPhysicalDevice choice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties choiceProps;
	bool foundPrimary;

	if (out != NULL) {
		choice = _vk2dPhysicalDeviceGetBestDevice(instance, out, preferredDevice, &foundPrimary);
		vkGetPhysicalDeviceProperties(choice, &choiceProps);

		// Check if we found one and print if it was discrete (dedicated) or otherwise
		if (choice != VK_NULL_HANDLE) {
            vk2dLogInfo(foundPrimary ? "Found discrete device %s [Vulkan %i.%i.%i]"
                                 : "Found integrated device %s [Vulkan %i.%i.%i]",
                    choiceProps.deviceName, VK_VERSION_MAJOR(choiceProps.apiVersion),
                    VK_VERSION_MINOR(choiceProps.apiVersion), VK_VERSION_PATCH(choiceProps.apiVersion));
			out->props = choiceProps;
			out->dev = choice;
			vkGetPhysicalDeviceFeatures(choice, &out->feats);
			vkGetPhysicalDeviceMemoryProperties(choice, &out->mem);
		} else {
			free(out);
			out = NULL;
			vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to find compatible device.");
		}
	} else {
	    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate device.");
	}

	return out;
}

VK2DMSAA vk2dPhysicalDeviceGetMSAA(VK2DPhysicalDevice physicalDevice) {
	VkSampleCountFlags counts = physicalDevice->props.limits.framebufferColorSampleCounts & physicalDevice->props.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK2D_MSAA_32X; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK2D_MSAA_16X; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK2D_MSAA_8X; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK2D_MSAA_4X; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK2D_MSAA_2X; }

	return VK2D_MSAA_1X;
}

void vk2dPhysicalDeviceFree(VK2DPhysicalDevice dev) {
	free(dev);
}