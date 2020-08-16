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
#include "VK2D/Image.h"

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

// Grabs a preferred present mode if available returning FIFO if its unavailable
static VkPresentModeKHR _vk2dRendererGetPresentMode(VkPresentModeKHR mode) {
	uint32_t i;
	for (i = 0; i < gRenderer->presentModeCount; i++)
		if (gRenderer->presentModes[i] == mode)
			return mode;
	return VK_PRESENT_MODE_FIFO_KHR;
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
		gRenderer->surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
		gRenderer->surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		_vk2dRendererGetSurfaceSize();
	}
}

static void _vk2dRendererDestroyWindowSurface() {
	vkDestroySurfaceKHR(gRenderer->vk, gRenderer->surface, VK_NULL_HANDLE);
	free(gRenderer->presentModes);
}

static void _vk2dRendererCreateSwapchain() {
	uint32_t i;

	gRenderer->config.screenMode = (VK2DScreenMode)_vk2dRendererGetPresentMode((VkPresentModeKHR)gRenderer->config.screenMode);
	VkSwapchainCreateInfoKHR  swapchainCreateInfoKHR = vk2dInitSwapchainCreateInfoKHR(
			gRenderer->surface,
			gRenderer->surfaceCapabilities,
			gRenderer->surfaceFormat,
			gRenderer->surfaceWidth,
			gRenderer->surfaceHeight,
			(VkPresentModeKHR)gRenderer->config.screenMode,
			VK_NULL_HANDLE,
			gRenderer->surfaceCapabilities.minImageCount + (gRenderer->config.screenMode == sm_TripleBuffer ? 1 : 0)
			);
	VkBool32 supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(gRenderer->pd->dev, gRenderer->pd->QueueFamily.graphicsFamily, gRenderer->surface, &supported);
	if (vk2dErrorInline(supported != VK_TRUE ? -1 : VK_SUCCESS))
		vkCreateSwapchainKHR(gRenderer->ld->dev, &swapchainCreateInfoKHR, VK_NULL_HANDLE, &gRenderer->swapchain);

	vk2dErrorCheck(vkGetSwapchainImagesKHR(gRenderer->ld->dev, gRenderer->swapchain, &gRenderer->swapchainImageCount, VK_NULL_HANDLE));
	gRenderer->swapchainImageViews = malloc(gRenderer->swapchainImageCount * sizeof(VkImageView));
	gRenderer->swapchainImages = malloc(gRenderer->swapchainImageCount * sizeof(VkImage));
	if (vk2dPointerCheck(gRenderer->swapchainImageViews) && vk2dPointerCheck(gRenderer->swapchainImages)) {
		vk2dErrorCheck(vkGetSwapchainImagesKHR(gRenderer->ld->dev, gRenderer->swapchain, &gRenderer->swapchainImageCount, gRenderer->swapchainImages));

		for (i = 0; i < gRenderer->swapchainImageCount; i++) {
			VkImageViewCreateInfo imageViewCreateInfo = vk2dInitImageViewCreateInfo(gRenderer->swapchainImages[i], gRenderer->surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
			vk2dErrorCheck(vkCreateImageView(gRenderer->ld->dev, &imageViewCreateInfo, VK_NULL_HANDLE, &gRenderer->swapchainImageViews[i]));
		}
	}

	vk2dLogMessage("Swapchain (%i images) initialized...", swapchainCreateInfoKHR.minImageCount);
}

static void _vk2dRendererDestroySwapchain() {
	uint32_t i;
	for (i = 0; i < gRenderer->swapchainImageCount; i++)
		vkDestroyImageView(gRenderer->ld->dev, gRenderer->swapchainImageViews[i], VK_NULL_HANDLE);

	vkDestroySwapchainKHR(gRenderer->ld->dev, gRenderer->swapchain, VK_NULL_HANDLE);
}

static void _vk2dRendererCreateColourResources() {
	if (gRenderer->config.msaa != msaa_1x) {
		gRenderer->msaaImage = vk2dImageCreate(
				gRenderer->ld,
				gRenderer->surfaceWidth,
				gRenderer->surfaceHeight,
				gRenderer->surfaceFormat.format,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				(VkSampleCountFlagBits) gRenderer->config.msaa);
		vk2dLogMessage("Colour resources initialized...");
	} else {
		vk2dLogMessage("Colour resources not enabled...");
	}
}

static void _vk2dRendererDestroyColourResources() {
	if (gRenderer->msaaImage != NULL)
		vk2dImageFree(gRenderer->msaaImage);
	gRenderer->msaaImage = NULL;
}

static void _vk2dRendererCreateDepthStencilImage() {
	// First order of business - find a good stencil format
	VkFormat formatAttempts[] = {
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
	};
	const uint32_t count = 3;
	uint32_t i;
	gRenderer->dsiAvailable = false;

	for (i = 0; i < count && !gRenderer->dsiAvailable; i++) {
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(gRenderer->pd->dev, formatAttempts[i], &formatProperties);

		if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			gRenderer->dsiFormat = formatAttempts[i];
			gRenderer->dsiAvailable = true;
		}
	}
	if (vk2dErrorInline(gRenderer->dsiAvailable == false ? -1 : 0)) {
		// Create the image itself
		gRenderer->dsi = vk2dImageCreate(
				gRenderer->ld,
				gRenderer->surfaceWidth,
				gRenderer->surfaceHeight,
				gRenderer->dsiFormat,
				VK_IMAGE_ASPECT_DEPTH_BIT,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				(VkSampleCountFlagBits)gRenderer->config.msaa);
		vk2dLogMessage("Depth stencil image initialized...");
	} else {
		vk2dLogMessage("Depth stencil image unavailable...");
	}
}

static void _vk2dRendererDestroyDepthStencilImage() {
	if (gRenderer->dsiAvailable)
		vk2dImageFree(gRenderer->dsi);
}

static void _vk2dRendererCreateRenderPass() {
	vk2dLogMessage("Render pass initialized...");
}

static void _vk2dRendererDestroyRenderPass() {

}

static void _vk2dRendererCreateDescriptorSetLayout() {
	vk2dLogMessage("Descriptor set layout initialized...");
}

static void _vk2dRendererDestroyDescriptorSetLayout() {

}

static void _vk2dRendererCreatePipelines() {
	vk2dLogMessage("Pipelines initialized...");
}

static void _vk2dRendererDestroyPipelines() {

}

static void _vk2dRendererCreateFrameBuffer() {
	vk2dLogMessage("Framebuffers initialized...");
}

static void _vk2dRendererDestroyFrameBuffer() {

}

static void _vk2dRendererCreateUniformBuffers() {
	vk2dLogMessage("UBO initialized...");
}

static void _vk2dRendererDestroyUniformBuffers() {

}

static void _vk2dRendererCreateDescriptorPool() {
	vk2dLogMessage("Descriptor pool initialized...");
}

static void _vk2dRendererDestroyDescriptorPool() {

}

static void _vk2dRendererCreateDescriptorSets() {
	vk2dLogMessage("Descriptor sets initialized...");
}

static void _vk2dRendererDestroyDescriptorSets() {

}

static void _vk2dRendererCreateSynchronization() {
	vk2dLogMessage("Synchronization initialized...");
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
		gRenderer->config.msaa = maxMSAA >= msaa ? msaa : maxMSAA;
		gRenderer->config.textureDetail = textureDetail;
		gRenderer->config.screenMode = screenMode;

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
		_vk2dRendererDestroySynchronization();
		_vk2dRendererDestroyDescriptorSets();
		_vk2dRendererDestroyDescriptorPool();
		_vk2dRendererDestroyUniformBuffers();
		_vk2dRendererDestroyFrameBuffer();
		_vk2dRendererDestroyPipelines();
		_vk2dRendererDestroyDescriptorSetLayout();
		_vk2dRendererDestroyRenderPass();
		_vk2dRendererDestroyDepthStencilImage();
		_vk2dRendererDestroyColourResources();
		_vk2dRendererDestroySwapchain();
		_vk2dRendererDestroyWindowSurface();
		_vk2dRendererDestroyDebug();

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