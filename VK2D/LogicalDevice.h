/// \file LogicalDevice.h
/// \author Paolo Mazzon
/// \brief Declares functionality as it pertains to VkDevice
#pragma once
#include <vulkan/vulkan.h>
#include "VK2D/Constants.h"

/// \brief Logical device that is essentially a wrapper of VkDevice
struct VK2DLogicalDevice {
	VkDevice dev;  ///< Logical device
	VkQueue queue; ///< Queue for command buffers
	VkCommandPool pool[VK2D_DEVICE_COMMAND_POOLS]; ///< Command pools to cycle through
};