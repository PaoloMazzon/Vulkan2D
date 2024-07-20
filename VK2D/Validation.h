/// \file Validation.h
/// \author Paolo Mazzon
/// \brief Some tools for managing errors
#pragma once
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Prints a log message if VKRE_VERBOSE_STDOUT is enabled
void vk2dLog(const char* fmt, ...);

/// \brief Raises a status problem of some sort
void vk2dRaise(VK2DStatus result, const char* fmt, ...);

/// \brief Gets the current renderer status to check for errors and the like
VK2DStatus vk2dGetStatus();

/// \brief Creates validation synchronization primitives
void vk2dValidationBegin();

/// \brief Cleans up validation synchronization primitives
void vk2dValidationEnd();

/// \brief Used internally to handle debugging callbacks
VKAPI_ATTR VkBool32 VKAPI_CALL _vk2dDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t sourceObject, size_t location, int32_t messageCode, const char* layerPrefix, const char* message, void* data);

#ifdef __cplusplus
};
#endif