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

/// \brief Backend for the reErrorCheck macro
bool _vk2dErrorRaise(VkResult result, const char* function, int line, const char* varname);

/// \brief Backend for the rePointerCheck macro (don't use this, use the macro)
bool _vk2dPointerCheck(void* ptr, const char* function, int line, const char* varname);

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

/// \brief Checks if a pointer exists and returns true if it does
#define vk2dPointerCheck(ptr) ((ptr) == NULL ? _vk2dPointerCheck(ptr, __func__, __LINE__, #ptr) : 1)

/// \brief Checks for an error without returning anything
#define vk2dErrorCheck(vkresult) {VkResult tmpres = (vkresult);\
if (tmpres < 0) _vk2dErrorRaise(tmpres, __func__, __LINE__, #vkresult);}

/// \brief Returns false if an error has occurred, printing that error as well
#define vk2dErrorInline(vkresult) (vkresult) < 0 ? _vk2dErrorRaise(vkresult, __func__, __LINE__, #vkresult) : true

/// \brief Used internally to handle debugging callbacks
VKAPI_ATTR VkBool32 VKAPI_CALL _vk2dDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t sourceObject, size_t location, int32_t messageCode, const char* layerPrefix, const char* message, void* data);

#ifdef __cplusplus
};
#endif