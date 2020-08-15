/// \file PhysicalDevice.c
/// \author Paolo Mazzon
#include "VK2D/PhysicalDevice.h"
#include "VK2D/Initializers.h"
#include "VK2D/Validation.h"
#include <malloc.h>
#include "VK2D/Constants.h"

static VkPhysicalDevice* _getPhysicalDevices(VkInstance instance, uint32_t *size) {
	VkPhysicalDevice *devices;

	// Find out how many devices we have, allocate that amount, then fill in the blanks
	vk2dErrorCheck(vkEnumeratePhysicalDevices(instance, size, NULL));
	devices = malloc(sizeof(VkPhysicalDevice) * (*size));
	vk2dErrorCheck(vkEnumeratePhysicalDevices(instance, size, devices));

	return devices;
}

static bool _physicalDeviceSupportsQueueFamilies(VkInstance instance, VkPhysicalDevice dev, VK2DPhysicalDevice out) {
	uint32_t queueFamilyCount = 0;
	uint32_t i;
	VkQueueFamilyProperties* queueList;
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, NULL);
	queueList = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueList);

	for (i = 0; i < queueFamilyCount; i++) {
		if (queueList[i].queueCount > 0) {
			bool cpu = false;
			bool gfx = false;
			if (queueList[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
				out->QueueFamily.computeFamily = i;
				cpu = true;
			}
			if (queueList[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				out->QueueFamily.graphicsFamily = i;
				gfx = true;
			}
			if (gfx && cpu) {
				free(queueList);
				return true;
			}
		}
	}

	free(queueList);
	return false;
}

// Checks if a device is supported, loading all the queue families if so
static bool _physicalDeviceSupported(VkInstance instance, VkPhysicalDevice dev, VkPhysicalDeviceProperties *props, VK2DPhysicalDevice out) {
	// You can add device requirements here
	return _physicalDeviceSupportsQueueFamilies(instance, dev, out);
}

VkPhysicalDeviceProperties *vk2dPhysicalDeviceGetList(VkInstance instance, uint32_t *size) {
	VkPhysicalDevice *devs = _getPhysicalDevices(instance, size);
	VkPhysicalDeviceProperties *props = NULL;

	if (vk2dPointerCheck(devs)) {
		props = malloc(sizeof(VkPhysicalDeviceProperties) * (*size));
		if (vk2dPointerCheck(props)) {
			uint32_t i;
			for (i = 0; i < *size; i++)
				vkGetPhysicalDeviceProperties(devs[i], &props[i]);
		}
	}

	free(devs);
	return props;
}

VK2DPhysicalDevice vk2dPhysicalDeviceFind(VkInstance instance, int32_t preferredDevice) {
	VK2DPhysicalDevice out = malloc(sizeof(struct VK2DPhysicalDevice));
	uint32_t devCount;
	VkPhysicalDevice *devs = _getPhysicalDevices(instance, &devCount);
	bool foundPrimary = false;
	VkPhysicalDevice choice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties choiceProps;

	// Attempt to find a good non-integrated gpu
	if (vk2dPointerCheck(devs) && vk2dPointerCheck(out) && preferredDevice == VK2D_DEVICE_BEST_FIT) {
		uint32_t i;
		for (i = 0; i < devCount && !foundPrimary; i++) {
			vkGetPhysicalDeviceProperties(devs[i], &choiceProps);
			if (_physicalDeviceSupported(instance, devs[i], &choiceProps, out)) {
				if (choiceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
					foundPrimary = true;
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
			vk2dErrorCheck(-1);
			vk2dLogMessage("Device \"%i\" out of range.", preferredDevice);
			free(out);
			out = NULL;
		}
	}

	// Check if we found one and print if it was discrete (dedicated) or otherwise
	if (choice != VK_NULL_HANDLE) {
		vk2dLogMessage(foundPrimary ? "Found discrete device %s" : "Found integrated device %s", choiceProps.deviceName);
		out->props = choiceProps;
		out->dev = choice;
		vkGetPhysicalDeviceFeatures(choice, &out->feats);
		vkGetPhysicalDeviceMemoryProperties(choice, &out->mem);
	} else {
		free(out);
		out = NULL;
		vk2dErrorCheck(-1);
	}

	free(devs);
	return out;
}

VK2DMSAA vk2dPhysicalDeviceGetMSAA(VK2DPhysicalDevice physicalDevice) {
	VkSampleCountFlags counts = physicalDevice->props.limits.framebufferColorSampleCounts & physicalDevice->props.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return msaa_32x; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return msaa_16x; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return msaa_8x; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return msaa_4x; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return msaa_2x; }

	return msaa_1x;
}

void vk2dPhysicalDeviceFree(VK2DPhysicalDevice dev) {
	free(dev);
}