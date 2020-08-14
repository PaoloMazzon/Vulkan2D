/// \file Constants.c
/// \author Paolo Mazzon
#include <vulkan/vulkan.h>

const int32_t VK2D_DEVICE_BEST_FIT = -1;

const VkApplicationInfo VK2D_DEFAULT_CONFIG = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		VK_NULL_HANDLE,
		"VK2D",
		VK_MAKE_VERSION(1, 0, 0),
		"VK2D Renderer",
		VK_MAKE_VERSION(1, 0, 0),
		VK_MAKE_VERSION(1, 2, 0)
};