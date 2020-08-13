/// \file Renderer.h
/// \author Paolo Mazzon
/// \brief The main renderer that handles all rendering
#pragma once
#include "VK2D/Structs.h"
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

/// \brief Core rendering data, don't modify values unless you know what you're doing
struct VK2DRenderer {
	// Devices/core functionality (these have short names because they're constantly referenced)
	VK2DPhysicalDevice pd;       ///< Physical device (gpu)
	VK2DLogicalDevice ld;        ///< Logical device
	VkInstance vk;               ///< Core vulkan instance
	VkDebugReportCallbackEXT dr; ///< Debug information

	// Configurable options
	VK2DMSAA msaa;               ///< Current MSAA
	VK2DTextureDetail texDetail; ///< Current texture detail level
	VK2DScreenMode screenMode;   ///< Current screen mode
	// TODO: Stick swapchain stuff in its own file
	// TODO: Abtract pipelines into their own file
};

/// \brief Initializes VK2D's renderer
/// \param window An SDL window created with the flag SDL_WINDOW_VULKAN
/// \param textureDetail Level of detail for textures
/// \param screenMode How to present the screen
/// \param msaa Number of samples per pixel
/// \return Returns weather or not the function was successful, less than zero is an error
///
/// GPUs are not guaranteed to support certain screen modes and msaa levels (integrated
/// gpus often don't usually support triple buffering, 32x msaa is not terribly common), so if
/// you request something that isn't supported, the next best thing is used in its place.
int32_t vk2dRendererInit(SDL_Window *window, VK2DTextureDetail textureDetail, VK2DScreenMode screenMode, VK2DMSAA msaa);

/// \brief Frees resources used by the renderer
void vk2dRendererQuit();