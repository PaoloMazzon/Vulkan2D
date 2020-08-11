/// \file PhysicalDevice.h
/// \author Paolo Mazzon
/// \brief Tools for managing VkPhysicalDevice
#pragma once
#include "VK2D/Types.h"

/// \brief Groups up a couple things related to VkPhysicalDevice
struct VK2DPhysicalDevice {
	VkPhysicalDevice dev; ///< Internal vulkan pointer
	struct {
		uint32_t graphicsFamily; ///< Queue family for graphics pipeline
		uint32_t computeFamily;  ///< Queue family for compute pipeline
	} QueueFamily;               ///< Nicely groups up queue families
	VkPhysicalDeviceMemoryProperties mem; ///< Memory properties of this device
	VkPhysicalDeviceFeatures feats;       ///< Features of this device
	VkPhysicalDeviceProperties props;     ///< Device properties
};

/// \brief Finds and returns a physical device or NULL
/// \param instance Vulkan instance
/// \param preferredDevice Either a preferred physical device index or VK2D_DEVICE_BEST_FIT
/// \return Returns the specified device or NULL if it fails
VK2DPhysicalDevice vk2dPhysicalDeviceFind(VkInstance instance, int32_t preferredDevice);

/// \brief Returns a list of device properties should you want to pick one specifically
/// \param instance Vulkan instance
/// \param size A pointer to a uint32_t where the size of the returned vector will be stored
/// \return Returns a vector of VkPhysicalDeviceProperties that will need to be freed
VkPhysicalDeviceProperties *vk2dPhysicalDeviceGetList(VkInstance instance, uint32_t *size);

/// \brief Finds and returns the highest available MSAA available on a given device
/// \param physicalDevice Device to find the MSAA max for
/// \return Returns VkSampleCountFlagBits representing the max MSAA (its just an int like 1, 2, 4, 8...)
VkSampleCountFlagBits vk2dPhysicalDeviceGetMSAA(VK2DPhysicalDevice physicalDevice);

/// \brief Frees a physical device
/// \param dev Device to be freed
void vk2dPhysicalDeviceFree(VK2DPhysicalDevice dev);