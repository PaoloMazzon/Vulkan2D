/// \file PhysicalDevice.h
/// \author Paolo Mazzon
/// \brief Tools for managing VkPhysicalDevice
#pragma once
#include <vulkan/vulkan.h>

/// \brief Groups up a couple things related to VkPhysicalDevice
struct VK2DPhysicalDevice {
	VkPhysicalDevice pd; ///< Internal vulkan pointer
	struct {
		uint32_t presentFamily;  ///< KHR family
		uint32_t graphicsFamily; ///< Queue family for graphics pipeline
		uint32_t computeFamily;  ///< Queue family for compute pipeline
	} QueueFamily;               ///< Nicely groups up queue families
	VkPhysicalDeviceMemoryProperties mem; ///< Memory properties of this device
	VkPhysicalDeviceFeatures feats;       ///< Features of this device
	VkPhysicalDeviceProperties props;     ///< Device properties
};