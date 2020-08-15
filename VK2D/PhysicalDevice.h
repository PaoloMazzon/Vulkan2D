/// \file PhysicalDevice.h
/// \author Paolo Mazzon
/// \brief Tools for managing VkPhysicalDevice
#pragma once
#include "VK2D/Structs.h"
#include <vulkan/vulkan.h>

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
/// \param preferredDevice Either a preferred physical device index selected with vk2dPhysicalDeviceGetList or VK2D_DEVICE_BEST_FIT
/// \return Returns the specified device or NULL if it fails
///
/// Even though this is a high performance renderer, VK2D still only supports one GPU
/// at a time because the benefit is very minimal for 2D games at an extremely high cost
/// in complexity. If you want to specify a GPU, use vk2dPhysicalDeviceGetList to get the
/// list of device information (which contains their name and some other things). Should
/// you choose not to specify a device, VK2D will either use the only available GPU or
/// the first dedicated graphics card it finds (for example, it would choose a GTX 1080
/// over Intel HD 4600). GPUs are still arbitrary, however, and for example if you had a
/// GTX 2080ti and 1060 in your system but it finds the 1060 first, it will choose the 1060.
VK2DPhysicalDevice vk2dPhysicalDeviceFind(VkInstance instance, int32_t preferredDevice);

/// \brief Returns a list of device properties should you want to pick one specifically
/// \param instance Vulkan instance
/// \param size Pointer to a uint32_t where the size of the returned vector will be stored
/// \return Returns a vector of VkPhysicalDeviceProperties that will need to be freed
VkPhysicalDeviceProperties *vk2dPhysicalDeviceGetList(VkInstance instance, uint32_t *size);

/// \brief Finds and returns the highest available MSAA available on a given device
/// \param physicalDevice Device to find the MSAA max for
/// \return Returns VkSampleCountFlagBits representing the max MSAA (its just an int like 1, 2, 4, 8...)
VK2DMSAA vk2dPhysicalDeviceGetMSAA(VK2DPhysicalDevice physicalDevice);

/// \brief Frees a physical device
/// \param dev Device to be freed
void vk2dPhysicalDeviceFree(VK2DPhysicalDevice dev);