/// \file Constants.h
/// \author Paolo Mazzon
/// \brief Defines some constants
#pragma once
#include <VK2D/Structs.h>
#include <vulkan/vulkan.h>

/// Tells vk2dPhysicalDeviceFind to use the best device it finds (basically just the first non-integrated it finds that meets criteria)
extern const int32_t VK2D_DEVICE_BEST_FIT;

/// Default configuration of this renderer
extern const VkApplicationInfo VK2D_DEFAULT_CONFIG;