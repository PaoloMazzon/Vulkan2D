/// \file Renderer.c
/// \author Paolo Mazzon
#include <vulkan/vulkan.h>
#include <SDL2/SDL_vulkan.h>
#include "VK2D/Renderer.h"
#include "VK2D/BuildOptions.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include "VK2D/Constants.h"
#include "VK2D/PhysicalDevice.h"
#include "VK2D/LogicalDevice.h"

/******************************* Globals *******************************/

// For debugging
PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT;

// For everything
VK2DRenderer gRenderer = NULL;

#ifdef VK2D_ENABLE_DEBUG
static const char* EXTENSIONS[] = {
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};
static const char* LAYERS[] = {
		"VK_LAYER_KHRONOS_validation"
};
static const int LAYER_COUNT = 1;
static const int EXTENSION_COUNT = 1;
#else // VK2D_ENABLE_DEBUG
static const char* EXTENSIONS[] = {

};
static const char* LAYERS[] = {

};
static const int LAYER_COUNT = 0;
static const int EXTENSION_COUNT = 0;
#endif // VK2D_ENABLE_DEBUG

/******************************* Internal functions *******************************/

static void _vk2dRendererCreateDebug() {
#ifdef VK2D_ENABLE_DEBUG
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(gRenderer->vk, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(gRenderer->vk, "vkDestroyDebugReportCallbackEXT");;

	if (vk2dPointerCheck(fvkCreateDebugReportCallbackEXT) && vk2dPointerCheck(fvkDestroyDebugReportCallbackEXT)) {
		VkDebugReportCallbackCreateInfoEXT callbackCreateInfoEXT = vk2dInitDebugReportCallbackCreateInfoEXT(_vk2dDebugCallback);
		fvkCreateDebugReportCallbackEXT(gRenderer->vk, &callbackCreateInfoEXT, VK_NULL_HANDLE, &gRenderer->dr);
	}
#endif // VK2D_ENABLE_DEBUG
}

static void _vk2dRendererDestroyDebug() {
#ifdef VK2D_ENABLE_DEBUG
	fvkDestroyDebugReportCallbackEXT(gRenderer->vk, gRenderer->dr, VK_NULL_HANDLE);
#endif // VK2D_ENABLE_DEBUG
}

static void _vk2dRendererGetSurfaceSize() {
	if (gRenderer->surfaceCapabilities.currentExtent.width == UINT32_MAX || gRenderer->surfaceCapabilities.currentExtent.height == UINT32_MAX) {
		SDL_Vulkan_GetDrawableSize(gRenderer->window, (void*)&gRenderer->surfaceWidth, (void*)&gRenderer->surfaceHeight);
	} else {
		gRenderer->surfaceWidth = gRenderer->surfaceCapabilities.currentExtent.width;
		gRenderer->surfaceHeight = gRenderer->surfaceCapabilities.currentExtent.height;
	}
}

static void _vk2dRendererCreateWindowSurface() {
	// Create the surface then load up surface relevant values
	vk2dErrorCheck(SDL_Vulkan_CreateSurface(gRenderer->window, gRenderer->vk, &gRenderer->surface) == SDL_TRUE ? VK_SUCCESS : -1);
	vk2dErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->presentModeCount, VK_NULL_HANDLE));
	gRenderer->presentModes = malloc(sizeof(VkPresentModeKHR) * gRenderer->presentModeCount);

	if (vk2dPointerCheck(gRenderer->presentModes)) {
		vk2dErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->presentModeCount, gRenderer->presentModes));
		vk2dErrorCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->surfaceCapabilities));
		// You may want to search for a different format, but according to the Vulkan hardware database, 100% of systems support VK_FORMAT_B8G8R8A8_SRGB
		gRenderer->surfaceFormat = VK_FORMAT_B8G8R8A8_SRGB;
		_vk2dRendererGetSurfaceSize();
	}
}

static void _vk2dRendererDestroyWindowSurface() {
	vkDestroySurfaceKHR(gRenderer->vk, gRenderer->surface, VK_NULL_HANDLE);
	free(gRenderer->presentModes);
}

static void _vk2dRendererCreateSwapchain() {

}

static void _vk2dRendererDestroySwapchain() {

}

static void _vk2dRendererCreateColourResources() {

}

static void _vk2dRendererDestroyColourResources() {

}

static void _vk2dRendererCreateDepthStencilImage() {

}

static void _vk2dRendererDestroyDepthStencilImage() {

}

static void _vk2dRendererCreateRenderPass() {

}

static void _vk2dRendererDestroyRenderPass() {

}

static void _vk2dRendererCreateDescriptorSetLayout() {

}

static void _vk2dRendererDestroyDescriptorSetLayout() {

}

static void _vk2dRendererCreatePipelines() {

}

static void _vk2dRendererDestroyPipelines() {

}

static void _vk2dRendererCreateFrameBuffer() {

}

static void _vk2dRendererDestroyFrameBuffer() {

}

static void _vk2dRendererCreateUniformBuffers() {

}

static void _vk2dRendererDestroyUniformBuffers() {

}

static void _vk2dRendererCreateDescriptorPool() {

}

static void _vk2dRendererDestroyDescriptorPool() {

}

static void _vk2dRendererCreateDescriptorSets() {

}

static void _vk2dRendererDestroyDescriptorSets() {

}

static void _vk2dRendererCreateSynchronization() {

}

static void _vk2dRendererDestroySynchronization() {

}

// If the window is resized or minimized or whatever
static void _vk2dRendererResetSwapchain() {
	// Hang while minimized
	SDL_WindowFlags flags;
	flags = SDL_GetWindowFlags(gRenderer->window);
	while (flags & SDL_WINDOW_MINIMIZED) {
		flags = SDL_GetWindowFlags(gRenderer->window);
		SDL_PumpEvents();
	}
	vkDeviceWaitIdle(gRenderer->ld->dev);

	// Free swapchain
	_vk2dRendererDestroySwapchain();
	_vk2dRendererDestroyColourResources();
	_vk2dRendererDestroyDepthStencilImage();
	_vk2dRendererDestroyRenderPass();
	_vk2dRendererDestroyPipelines();
	_vk2dRendererDestroyFrameBuffer();
	_vk2dRendererDestroyUniformBuffers();
	_vk2dRendererDestroyDescriptorPool();
	_vk2dRendererDestroyDescriptorSets();

	// Restart swapchain
	_vk2dRendererGetSurfaceSize();
	_vk2dRendererCreateSwapchain();
	_vk2dRendererCreateColourResources();
	_vk2dRendererCreateDepthStencilImage();
	_vk2dRendererCreateRenderPass();
	_vk2dRendererCreatePipelines();
	_vk2dRendererCreateFrameBuffer();
	_vk2dRendererCreateUniformBuffers();
	_vk2dRendererCreateDescriptorPool();
	_vk2dRendererCreateDescriptorSets();
}

/******************************* User-visible functions *******************************/

int32_t vk2dRendererInit(SDL_Window *window, VK2DTextureDetail textureDetail, VK2DScreenMode screenMode, VK2DMSAA msaa) {
	gRenderer = calloc(1, sizeof(struct VK2DRenderer));
	int32_t errorCode = 0;
	uint32_t totalExtensionCount, i, sdlExtensions;
	const char** totalExtensions;

	// Print all available layers
	VkLayerProperties *systemLayers;
	uint32_t systemLayerCount;
	vkEnumerateInstanceLayerProperties(&systemLayerCount, VK_NULL_HANDLE);
	systemLayers = malloc(sizeof(VkLayerProperties) * systemLayerCount);
	vkEnumerateInstanceLayerProperties(&systemLayerCount, systemLayers);
	vk2dLogMessage("Available layers: ");
	for (i = 0; i < systemLayerCount; i++)
		vk2dLogMessage("  - %s", systemLayers[i].layerName);
	vk2dLogMessage("");
	free(systemLayers);

	// Find number of total number of extensions
	SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensions, VK_NULL_HANDLE);
	totalExtensionCount = sdlExtensions + EXTENSION_COUNT;
	totalExtensions = malloc(totalExtensionCount * sizeof(char*));

	if (vk2dPointerCheck(gRenderer)) {
		// Load extensions
		SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensions, totalExtensions);
		for (i = sdlExtensions; i < totalExtensionCount; i++) totalExtensions[i] = EXTENSIONS[i - sdlExtensions];

		// Log all used extensions
		vk2dLogMessage("Vulkan Enabled Extensions: ");
		for (i = 0; i < totalExtensionCount; i++)
			vk2dLogMessage(" - %s", totalExtensions[i]);
		vk2dLogMessage(""); // Newline

		// Create instance, physical, and logical device
		VkInstanceCreateInfo instanceCreateInfo = vk2dInitInstanceCreateInfo((void*)&VK2D_DEFAULT_CONFIG, LAYERS, LAYER_COUNT, totalExtensions, totalExtensionCount);
		vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &gRenderer->vk);
		gRenderer->pd = vk2dPhysicalDeviceFind(gRenderer->vk, VK2D_DEVICE_BEST_FIT);
		gRenderer->ld = vk2dLogicalDeviceCreate(gRenderer->pd, false, true);
		gRenderer->window = window;

		// Assign user settings, except for screen mode which will be handled later
		VK2DMSAA maxMSAA = vk2dPhysicalDeviceGetMSAA(gRenderer->pd);
		gRenderer->msaa = maxMSAA >= msaa ? msaa : maxMSAA;
		gRenderer->texDetail = textureDetail;

		// Initialize subsystems
		_vk2dRendererCreateDebug();
		_vk2dRendererCreateWindowSurface();
		_vk2dRendererCreateSwapchain();
		_vk2dRendererCreateColourResources();
		_vk2dRendererCreateDepthStencilImage();
		_vk2dRendererCreateRenderPass();
		_vk2dRendererCreateDescriptorSetLayout();
		_vk2dRendererCreatePipelines();
		_vk2dRendererCreateFrameBuffer();
		_vk2dRendererCreateUniformBuffers();
		_vk2dRendererCreateDescriptorPool();
		_vk2dRendererCreateDescriptorSets();
		_vk2dRendererCreateSynchronization();
	} else {
		errorCode = -1;
	}

	return errorCode;
}

void vk2dRendererQuit() {
	if (gRenderer != NULL) {
		// Destroy subsystems
		_vk2dRendererDestroyDebug();
		_vk2dRendererDestroyWindowSurface();
		_vk2dRendererDestroySwapchain();
		_vk2dRendererDestroyColourResources();
		_vk2dRendererDestroyDepthStencilImage();
		_vk2dRendererDestroyRenderPass();
		_vk2dRendererDestroyDescriptorSetLayout();
		_vk2dRendererDestroyPipelines();
		_vk2dRendererDestroyFrameBuffer();
		_vk2dRendererDestroyUniformBuffers();
		_vk2dRendererDestroyDescriptorPool();
		_vk2dRendererDestroyDescriptorSets();
		_vk2dRendererDestroySynchronization();

		// Destroy core bits
		vk2dLogicalDeviceFree(gRenderer->ld);
		vk2dPhysicalDeviceFree(gRenderer->pd);

		free(gRenderer);
		gRenderer = NULL;
	}
}

VK2DRenderer vk2dRendererGetPointer() {
	return gRenderer;
}

void vk2dRendererResetSwapchain() {
	gRenderer->resetSwapchain = true;
}