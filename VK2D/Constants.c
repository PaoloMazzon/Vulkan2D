/// \file Constants.c
/// \author Paolo Mazzon
#include <vulkan/vulkan.h>

/// Tells vk2dPhysicalDeviceFind to use the best device it finds (basically just the first non-integrated it finds that meets criteria)
const int32_t VK2D_DEVICE_BEST_FIT = -1;

/// Default configuration of this renderer
const VkApplicationInfo VK2D_DEFAULT_CONFIG = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		VK_NULL_HANDLE,
		"VK2D",
		VK_MAKE_VERSION(1, 0, 0),
		"VK2D Renderer",
		VK_MAKE_VERSION(1, 0, 0),
		VK_MAKE_VERSION(1, 2, 0)
};