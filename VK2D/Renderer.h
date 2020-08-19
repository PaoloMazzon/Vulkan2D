/// \file Renderer.h
/// \author Paolo Mazzon
/// \brief The main renderer that handles all rendering
#pragma once
#include "VK2D/Structs.h"
#include "Constants.h"
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
	VK2DRendererConfig config;    ///< User config
	VK2DRendererConfig newConfig; ///< In the event that its updated, we only swap out when we're ready to reset the swapchain
	bool resetSwapchain;          ///< If true, the swapchain (effectively the whole thing) will reset on the next rendered frame
	VK2DImage msaaImage;          ///< In case MSAA is enabled

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
	VkSwapchainKHR swapchain;         ///< Swapchain (manages images and presenting to screen)
	VkImage *swapchainImages;         ///< Images of the swapchain
	VkImageView *swapchainImageViews; ///< Image views for the swapchain images
	uint32_t swapchainImageCount;     ///< Number of images in the swapchain
	VkRenderPass renderPass;          ///< The render pass
	VkFramebuffer *framebuffers;      ///< Framebuffers for the swapchain images

	// Depth stencil image things
	bool dsiAvailable;  ///< Whether or not the depth stencil image is available
	VkFormat dsiFormat; ///< Format of the depth stencil image
	VK2DImage dsi;      ///< Depth stencil image

	// Pipelines
	VK2DPipeline texPipe;                   ///< Pipeline for rendering textures
	VK2DPipeline primFillPipe;              ///< Pipeline for rendering filled shapes
	VK2DPipeline primLinePipe;              ///< Pipeline for rendering shape outlines
	VK2DPipeline *customPipes;              ///< User defined shaders/pipelines
	uint32_t pipeCount;                     ///< Number of user defined pipelines
	VK2DCustomPipelineInfo *customPipeInfo; ///< Information required to recreate user pipelines

	// Uniform things
	VkDescriptorSetLayout duslt;   ///< Default uniform descriptor set layout for textures
	VkDescriptorSetLayout dusls;   ///< Default uniform descriptor set layout for shapes
	VkDescriptorPool descPoolTex;  ///< Pool for texture descriptors
	VkDescriptorPool descPoolPrim; ///< Pool for shape descriptors
	VkDescriptorSet descSetsTex;   ///< Texture descriptor sets
	VkDescriptorSet descSetsPrim;  ///< Shape descriptor sets

	// One UBO per frame for testing
	/* In the future this should be one view/projection matrix per frame
	 * and one model matrix per instance, all of which set via push constants */
	VK2DUniformBufferObject *ubos; ///< One ubo per swapchain image
	VK2DBuffer *uboBuffers;        ///< Buffers in memory for the UBOs
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
///
/// Something important to note is that by default the renderer has three graphics pipelines that
/// you can add to. Those three pipelines are as follows:
///
///  - Texture pipeline that uses VK2DVertexTexture as vertices
///  - Primitives pipeline that draws filled triangles that uses VK2DVertexColour as vertices
///  - Primitives pipeline that draws wireframe triangles that uses VK2DVertexColour as vertices
///
/// That should cover ~95% of all 2D drawing requirements, for specifics just check the shaders'
/// source code. Pipelines that are added by the user are tracked by the renderer and should
/// the swapchain need to be reconstructed (config change, window resize, user requested) the
/// renderer will recreate the pipelines without the user ever needing to get involved. This means
/// all pipeline settings and shaders are copied and stored inside the renderer should they need
/// to be remade.
int32_t vk2dRendererInit(SDL_Window *window, VK2DTextureDetail textureDetail, VK2DScreenMode screenMode, VK2DMSAA msaa);

/// \brief Frees resources used by the renderer
void vk2dRendererQuit();

/// \brief Gets the internal renderer's pointer
/// \return Returns the internal VK2DRenderer pointer (can be NULL)
///
/// This is meant to be more control and configuration for those who are comfortable
/// with Vulkan. It is not recommended that you use this function unless necessary.
VK2DRenderer vk2dRendererGetPointer();

/// \brief Gets the current user configuration of the renderer
/// \return Returns the structure containing the config information
///
/// This returns the *ACTUAL* user configuration, not what you've requested. If you've
/// requested for a setting that isn't available on the current device, this will return
/// what was actually used instead (for example, if you request 32x MSAA but only 8x was
/// available, 8x will be returned).
VK2DRendererConfig vk2dRendererGetConfig();

/// \brief Resets the renderer with a new configuration
/// \param config New render user configuration to use
///
/// Changes take effect when vk2dRendererResetSwapchain would normally take effect. That
/// also means vk2dRendererGetConfig will continue to return the same thing until this
/// configuration takes effect at the end of the frame.
void vk2dRendererSetConfig(VK2DRendererConfig config);

/// \brief Resets the rendering pipeline after the next frame is rendered
///
/// This is automatically done when Vulkan detects the window is no longer suitable,
/// but this is still available to do manually if you so desire.
void vk2dRendererResetSwapchain();
