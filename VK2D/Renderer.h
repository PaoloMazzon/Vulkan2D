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
///
/// Drawing and drawing synchronization is kind of tricky, so an in-depth explanation
/// is here for people looking to understand it and future me. At the start of each
/// frame a few things happen
///
///  - Renderer selects a command pool to use for this frame and resets it (there are VK2D_DEVICE_COMMAND_POOLS command pools that are cycled through)
///  - A primary command buffer is made from it and it starts a render pass, clearing the contents
///
/// After this, should you switch the render target, other things happen.
///
///  - All recorded secondary command buffers starting at `drawOffset` (in the `draws` variable) are executed in the render pass in the primary command buffer for the frame
///  - The render pass is ended, a new one is started for the render target (screen uses the swapchain's framebuffers, textures have a framebuffer if they're a target)
///  - `drawOffset` is updated to the current amount of secondary command buffers so the original ones are preserved and won't be executed twice in the future
///
/// That may happen any number of times in a frame. At the end of the frame, something similar happens:
///
///  - All recorded secondary command buffers starting at `drawOffset` are executed in the current render pass
///  - `drawOffset` is not updated because it will not be needed again this frame
///  - The screen is presented
///
/// Since the start of the frame starts a render pass in the screen's framebuffer and by the end of
/// the frame that render pass is guaranteed to have ended, the image should always be in the proper
/// KHR present source.
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
	vec4 colourBlend;             ///< For future use, will be used in all shaders via push constants to blend colours with another one (for on-the-fly changing of transparency/colour)

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
	VkSwapchainKHR swapchain;              ///< Swapchain (manages images and presenting to screen)
	VkImage *swapchainImages;              ///< Images of the swapchain
	VkImageView *swapchainImageViews;      ///< Image views for the swapchain images
	uint32_t swapchainImageCount;          ///< Number of images in the swapchain
	VkRenderPass renderPass;               ///< The render pass
	VkRenderPass midFrameSwapRenderPass;   ///< Render pass for mid-frame switching back to the swapchain as a target
	VkRenderPass externalTargetRenderPass; ///< Render pass for rendering to textures
	VkFramebuffer *framebuffers;           ///< Framebuffers for the swapchain images

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
	VkDescriptorSetLayout duslt; ///< Default uniform descriptor set layout for textures
	VkDescriptorSetLayout dusls; ///< Default uniform descriptor set layout for shapes
	VK2DDescCon *descConTex;     ///< Descriptor controllers for texture pipelines (1 per swapchain image)
	VK2DDescCon *descConPrim;    ///< Descriptor controllers for shapes pipelines (1 per swapchain image)

	// Frame synchronization
	uint32_t currentFrame;                 ///< Current frame being looped through
	uint32_t scImageIndex;                 ///< Swapchain image index to be rendered to this frame
	VkSemaphore *imageAvailableSemaphores; ///< Semaphores to signal when the image is ready
	VkSemaphore *renderFinishedSemaphores; ///< Semaphores to signal when rendering is done
	VkFence *inFlightFences;               ///< Fences for each frame
	VkFence *imagesInFlight;               ///< Individual images in flight

	// Command buffers for drawing management
	VkCommandBuffer **draws;        ///< List of lists (1 list per command pool, current one being drawCommandPool), inner list being a list of secondary command buffers containing draw commands
	uint32_t *drawListSize;         ///< Total size of command buffer list (All are valid command buffers, only drawCommandBuffers[i] are currently in use) (1 value per command pool)
	uint32_t *drawCommandBuffers;   ///< Amount of actual command buffers in the list (1 value per command pool)
	uint32_t drawCommandPool;       ///< Index of the logical device's command pool to pull from
	uint32_t drawOffset;            ///< Offset of where to execute from in the command buffer when starting a new render pass (in case render target is switched)
	VkCommandBuffer *primaryBuffer; ///< "Master" command buffer that does the render passes and executes the ones in `draws` (1 per command pool)

	// Render targeting info
	uint32_t targetSubPass;          ///< Current sub pass being rendered to
	VkRenderPass targetRenderPass;   ///< Current render pass being rendered to
	VkFramebuffer targetFrameBuffer; ///< Current framebuffer being rendered to

	// One UBO per frame for testing
	/* In the future this should be one view/projection matrix per frame
	 * and one model matrix per instance, all of which set via push constants */
	VK2DUniformBufferObject *ubos; ///< One ubo per swapchain image
	VK2DBuffer *uboBuffers;        ///< Buffers in memory for the UBOs
};

/// \brief Initializes VK2D's renderer
/// \param window An SDL window created with the flag SDL_WINDOW_VULKAN
/// \param config Initial renderer configuration settings
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
int32_t vk2dRendererInit(SDL_Window *window, VK2DRendererConfig config);

/// \brief Waits until current GPU tasks are done before moving on
///
/// Make sure you call this before freeing your assets in case they're still being used.
void vk2dRendererWait();

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

/// \brief Performs the tasks necessary to start rendering a frame (call before you start drawing)
void vk2dRendererStartFrame();

/// \brief Performs the tasks necessary to complete/present a frame (call once you're done drawing)
void vk2dRendererEndFrame();

/// \brief Returns the logical device being used by the renderer
/// \return Returns the current logical device
VK2DLogicalDevice vk2dRendererGetDevice();

/// \brief Changes the render target to a texture or the screen
/// \param target Target texture to switch to or VK2D_TARGET_SCREEN for the screen
/// \warning This can be computationally expensive so don't take this simple function lightly (it ends then starts a render pass)
void vk2dRendererSetTarget(VK2DTexture target);

/// \brief Renders a texture
/// \param tex Texture to draw
/// \param x x position in pixels from the top left of the window to draw it from
/// \param y y position in pixels from the top left of the window to draw it from
/// \param xscale Horizontal scale for drawing the texture (negative for flipped)
/// \param yscale Vertical scale for drawing the texture (negative for flipped)
/// \param rot Rotation to draw the texture (VK2D only uses radians)
void vk2dRendererDrawTex(VK2DTexture tex, float x, float y, float xscale, float yscale, float rot);

/// \brief Renders a polygon
/// \param polygon Polygon to draw
/// \param x x position in pixels from the top left of the window to draw it from
/// \param y y position in pixels from the top left of the window to draw it from
/// \param xscale Horizontal scale for drawing the polygon (negative for flipped)
/// \param yscale Vertical scale for drawing the polygon (negative for flipped)
/// \param rot Rotation to draw the polygon (VK2D only uses radians)
void vk2dRendererDrawPolygon(VK2DPolygon polygon, bool filled, float x, float y, float xscale, float yscale, float rot);


// TODO: Function for loading custom shaders