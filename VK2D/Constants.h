/// \file Constants.h
/// \author Paolo Mazzon
/// \brief Defines some constants
#pragma once
#include "VK2D/Structs.h"
#include <vulkan/vulkan.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Tells vk2dPhysicalDeviceFind to use the best device it finds (basically just the first non-integrated it finds that meets criteria)
extern const int32_t VK2D_DEVICE_BEST_FIT;

/// Default configuration of this renderer
extern const VkApplicationInfo VK2D_DEFAULT_CONFIG;

// This is a preprocessor because variable size arrays cannot be used in structs
/// Number of command pools a device has to cycle through
#define VK2D_DEVICE_COMMAND_POOLS ((uint32_t)3)

#ifdef __cplusplus
};
#endif