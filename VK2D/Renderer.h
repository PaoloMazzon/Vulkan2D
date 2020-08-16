/// \file Renderer.h
/// \author Paolo Mazzon
/// \brief The main renderer that handles all rendering
#pragma once
#include "VK2D/Structs.h"
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Core rendering data, don't modify values unless you know what you're doing
struct VK2DRenderer {
	// Devices/core functionality (these have short names because they're constantly referenced)
	VK2DPhysicalDevice pd;       ///< Physical device (gpu)
	VK2DLogicalDevice ld;        ///< Logical device
	VkInstance vk;               ///< Core vulkan instance
	VkDebugReportCallbackEXT dr; ///< Debug information

	// Configurable options
	VK2DRendererConfig config; ///< User config
	bool resetSwapchain;       ///< If true, the swapchain (effectively the whole thing) will reset on the next rendered frame

	// KHR Surface
	SDL_Window *window;                           ///< Window this renderer belongs to
	VkSurfaceKHR surface;                         ///< Window surface
	VkSurfaceCapabilitiesKHR surfaceCapabilities; ///< Capabilities of the surface
	VkPresentModeKHR *presentModes;               ///< All available present modes
	uint32_t presentModeCount;                    ///< Number of present modes
	VkSurfaceFormatKHR surfaceFormat;             ///< Window surface format
	uint32_t surfaceWidth;                        ///< Width of the surface
	uint32_t surfaceHeight;                       ///< Height of the surface

	// Swapchain
	VkSwapchainKHR swapchain; ///< Swapchain (manages images and presenting to screen)

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

/// \brief Gets the internal renderer's pointer
/// \return Returns the internal VK2DRenderer pointer (can be NULL)
///
/// This is meant to be more control and configuration for those who are comfortable
/// with Vulkan. It is not recommended that you use this function unless necessary.
VK2DRenderer vk2dRendererGetPointer();

/// \brief Resets the rendering pipeline after the next frame is rendered
///
/// This is automatically done when Vulkan detects the window is no longer suitable,
/// but this is still available to do manually if you so desire.
void vk2dRendererResetSwapchain();
