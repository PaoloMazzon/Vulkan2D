/// \file Renderer.h
/// \author Paolo Mazzon
/// \brief The main renderer that handles all rendering
#pragma once
#include "VK2D/Structs.h"
#include "Constants.h"
#include "VK2D/Camera.h"
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <VulkanMemoryAllocator/src/vk_mem_alloc.h>

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
	VmaAllocator vma;

	// User-end things
	VK2DRendererConfig config;            ///< User config
	VK2DRendererConfig newConfig;         ///< In the event that its updated, we only swap out when we're ready to reset the swapchain
	bool resetSwapchain;                  ///< If true, the swapchain (effectively the whole thing) will reset on the next rendered frame
	VK2DImage msaaImage;                  ///< In case MSAA is enabled
	vec4 colourBlend;                     ///< Used to modify colours (and transparency) of anything drawn. Passed via push constants.
	VkSampler textureSampler;             ///< Needed for textures
	VkViewport viewport;                  ///< Viewport to draw with
	bool enableTextureCameraUBO;          ///< If true, when drawing to a texture the UBO for the internal camera is used instead of the texture's UBO
	VK2DBlendMode blendMode;              ///< Current blend mode to draw with
	VK2DCameraSpec defaultCameraSpec;     ///< Default camera spec (spec for camera 0)
	VK2DCamera cameras[VK2D_MAX_CAMERAS]; ///< All cameras to be drawn to
	VK2DCameraIndex cameraLocked;         ///< If true, only the default camera will be drawn to

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
	bool procedStartFrame;                 ///< End frame things are only done if this is true and start frame things are only done if this is false

	// Pipelines
	VK2DPipeline texPipe;      ///< Pipeline for rendering textures
	VK2DPipeline primFillPipe; ///< Pipeline for rendering filled shapes
	VK2DPipeline primLinePipe; ///< Pipeline for rendering shape outlines
	uint32_t shaderListSize;   ///< Size of the list of customShaders
	VK2DShader *customShaders; ///< Custom shaders the user creates

	// Uniform things
	VkDescriptorSetLayout dslSampler;    ///< Descriptor set layout for texture samplers
	VkDescriptorSetLayout dslBufferVP;   ///< Descriptor set layout for the view-projection buffer
	VkDescriptorSetLayout dslBufferUser; ///< Descriptor set layout for user data buffers (custom shaders uniforms)
	VkDescriptorSetLayout dslTexture;    ///< Descriptor set layout for the textures
	VK2DDescCon descConSamplers;         ///< Descriptor controller for samplers
	VK2DDescCon descConVP;               ///< Descriptor controller for view projection buffers
	VK2DDescCon descConUser;             ///< Descriptor controller for user buffers
	VkDescriptorPool samplerPool;        ///< Sampler pool for 1 sampler
	VkDescriptorSet samplerSet;          ///< Sampler for all textures

	// Frame synchronization
	uint32_t currentFrame;                 ///< Current frame being looped through
	uint32_t scImageIndex;                 ///< Swapchain image index to be rendered to this frame
	VkSemaphore *imageAvailableSemaphores; ///< Semaphores to signal when the image is ready
	VkSemaphore *renderFinishedSemaphores; ///< Semaphores to signal when rendering is done
	VkFence *inFlightFences;               ///< Fences for each frame
	VkFence *imagesInFlight;               ///< Individual images in flight
	VkCommandBuffer *commandBuffer;        ///< Command buffers, recreated each frame

	// Render targeting info
	uint32_t targetSubPass;          ///< Current sub pass being rendered to
	VkRenderPass targetRenderPass;   ///< Current render pass being rendered to
	VkFramebuffer targetFrameBuffer; ///< Current framebuffer being rendered to
	VkImage targetImage;             ///< Current image being rendered to
	VkDescriptorSet targetUBOSet;    ///< UBO being used for rendering
	VK2DTexture target;              ///< Just for simplicity sake
	VK2DTexture *targets;            ///< List of all currently loaded textures targets (in case the MSAA is changed and the sample image needs to be reloaded)
	uint32_t targetListSize;         ///< Amount of elements in the list (only non-null elements count)

	// Optimization tools - if the renderer knows the proper set/pipeline/vbo is already bound it doesn't need to rebind it
	uint64_t prevSetHash; ///< Currently bound descriptor set
	VkBuffer prevVBO;     ///< Currently bound vertex buffer
	VkPipeline prevPipe;  ///< Currently bound pipeline

	// Makes drawing things simpler
	VK2DPolygon unitSquare;        ///< Used to draw rectangles
	VK2DPolygon unitSquareOutline; ///< Used to draw rectangle outlines
	VK2DPolygon unitCircle;        ///< Used to draw circles
	VK2DPolygon unitCircleOutline; ///< Used to draw circle outlines
	VK2DPolygon unitLine;          ///< Used to draw lines
	VK2DBuffer unitUBO;            ///< Used to draw to the whole screen
	VkDescriptorSet unitUBOSet;    ///< Descriptor Set for the unit ubo

	// Debugging tools
	double previousTime;     ///< Time that the current frame started
	double amountOfFrames;   ///< Number of frames needed to calculate frameTimeAverage
	double accumulatedTime;  ///< Total time of frames for average in ms
	double frameTimeAverage; ///< Average amount of time frames are taking over a second (in ms)
};

/// \brief Initializes VK2D's renderer
/// \param window An SDL window created with the flag SDL_WINDOW_VULKAN
/// \param config Initial renderer configuration settings
/// \return Returns weather or not the function was successful, less than zero is an error
///
/// GPUs are not guaranteed to support certain screen modes and msaa levels (integrated
/// gpus often don't support triple buffering, 32x msaa is not terribly common), so if
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
/// \warning This could be referred to as the ***DANGER ZONE***, read the documentation before trying anything
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
/// \param clearColour Colour to clear the screen to
/// \warning You may only call drawing functions after vk2dRendererStartFrame is called and before vk2dRendererEndFrame is called
	void vk2dRendererStartFrame(vec4 clearColour);

/// \brief Performs the tasks necessary to complete/present a frame (call once you're done drawing)
/// \warning You may only call drawing functions after vk2dRendererStartFrame is called and before vk2dRendererEndFrame is called
void vk2dRendererEndFrame();

/// \brief Returns the logical device being used by the renderer
/// \return Returns the current logical device
VK2DLogicalDevice vk2dRendererGetDevice();

/// \brief Changes the render target to a texture or the screen
/// \param target Target texture to switch to or VK2D_TARGET_SCREEN for the screen
/// \warning This can be computationally expensive so don't take this simple function lightly (it ends then starts a render pass)
/// \warning Any time you change the target to a texture, you absolutely must change the target back to VK2D_TARGET_SCREEN when you're done drawing or else you can expect a crash
///
/// If things aren't being drawn the way you expect them to be, you may be forgetting
/// to set the viewport. If you change targets to a texture smaller than the screen, you
/// likely want to change the viewport to the size of the texture as well or you will
/// get some strange effects.
void vk2dRendererSetTarget(VK2DTexture target);

/// \brief Sets the rendering blend mode (does nothing if VK2D_GENERATE_BLEND_MODES is disabled)
/// \param blendMode Blend mode to use for drawing
void vk2dRendererSetBlendMode(VK2DBlendMode blendMode);

/// \brief Gets the current blend mode (will return whatever was set with vk2dRendererSetBlendMode regardless of whether blend modes are enabled or not)
/// \return Returns the current blend mode
VK2DBlendMode vk2dRendererGetBlendMode();

/// \brief Sets the current colour modifier (Colour all pixels are blended with)
/// \param mod Colour mod to make current
void vk2dRendererSetColourMod(const vec4 mod);

/// \brief Gets the current colour modifier
/// \param dst Destination vector to place the current colour mod into
///
/// The vec4 is treated as an RGBA array
void vk2dRendererGetColourMod(vec4 dst);

/// \brief Allows you to enable or disable the use of the renderer's camera when drawing to textures
/// \param useCameraOnTextures If true, the renderer's camera will be used when drawing to textures
///
/// This is kind of unintuitive to explain with a quick sentence, so here is a very long explanation.
/// Whenever you create a texture that is meant to be drawn to, a view and projection matrix are made
/// for it that account for its internal width and height (in order to properly render things. Look
/// into model-view-projection matrices if you're interested). The renderer stores several of these
/// (one per swapchain image as to allow for multi-frame rendering, this is not something the user
/// needs to think about) that use the user-provided camera so you can have simple 2D camera controls.
/// Should you want to render your game to a texture before drawing it to screen (possibly for pixel-
/// perfect scaling or to apply fragment shaders) you can enable this to make the renderer use the
/// internal camera matrices instead of the texture ones which allows you to use your camera transformations
/// when you draw to your textures. If you choose to do this, you most likely want to make the camera's
/// virtual width and height equal to the texture's actual width and height.
void vk2dRendererSetTextureCamera(bool useCameraOnTextures);

/// \brief Gets the average amount of time frames are taking to process from the start of vk2dRendererStartFrame to the end of vk2dRendererEndFrame
/// \return Returns average frame time over a course of a second in ms (1000 / vk2dRendererGetAverageFrameTime() will give FPS)
double vk2dRendererGetAverageFrameTime();

/// \brief Sets the current camera settings
/// \param camera Camera settings to use
///
/// This changes the default camera of the renderer, this is effectively a safer version of calling vk2dCameraUpdate with
/// the default camera because this function does not allow the `w/hOnScreen` to change (the default camera's `w/hOnScreen`
/// should almost always be the window size - if you need something else create a new camera).
void vk2dRendererSetCamera(VK2DCameraSpec camera);

/// \brief Returns the camera spec of the default camera, this is equivalent to calling `vk2dCameraGetSpec(VK2D_DEFAULT_CAMERA)`.
/// \return Returns the current camera settings
VK2DCameraSpec vk2dRendererGetCamera();

/// \brief Locking cameras means that only the specified camera will be drawn to and all others ignored
/// \param cam The camera to designate as the "lock"
void vk2dRendererLockCameras(VK2DCameraIndex cam);

/// \brief Unlocking cameras means that all cameras will be drawn to again
void vk2dRendererUnlockCameras();

/// \brief Sets the current viewport (portion of the window that is drawn to)
/// \param x X in window to draw to
/// \param y Y in window to draw to
/// \param w Width to draw
/// \param h Height to draw
void vk2dRendererSetViewport(float x, float y, float w, float h);

/// \brief Gets the current viewport
/// \param x Will be given the current x
/// \param y Will be given the current y
/// \param w Will be given the current w
/// \param h Will be given the current h
void vk2dRendererGetViewport(float *x, float *y, float *w, float *h);

/// \brief Clears the current render target to the current renderer colour
/// \warning This will do nothing unless the VK2D_UNIT_GENERATION option is enabled
void vk2dRendererClear();

/// \brief Draws a rectangle using the current rendering colour
/// \param x X position to draw the rectangle
/// \param y Y position to draw the rectangle
/// \param w Width of the rectangle
/// \param h Height of the rectangle
/// \param r Rotation of the rectangle
/// \param ox X origin of rotation of the rectangle (in percentage)
/// \param oy Y origin of rotation of the rectangle (in percentage)
/// \warning This will do nothing unless the VK2D_UNIT_GENERATION option is enabled
void vk2dRendererDrawRectangle(float x, float y, float w, float h, float r, float ox, float oy);

/// \brief Draws a rectangle using the current rendering colour
/// \param x X position to draw the rectangle
/// \param y Y position to draw the rectangle
/// \param w Width of the rectangle
/// \param h Height of the rectangle
/// \param r Rotation of the rectangle
/// \param ox X origin of rotation of the rectangle (in percentage)
/// \param oy Y origin of rotation of the rectangle (in percentage)
/// \param lineWidth Width of the outline
/// \warning This will do nothing unless the VK2D_UNIT_GENERATION option is enabled
void vk2dRendererDrawRectangleOutline(float x, float y, float w, float h, float r, float ox, float oy, float lineWidth);

/// \brief Draws a circle using the current rendering colour
/// \param x X position of the circle's center
/// \param y Y position of the circle's center
/// \param r Radius in pixels of the circle
/// \warning This will do nothing unless the VK2D_UNIT_GENERATION option is enabled
void vk2dRendererDrawCircle(float x, float y, float r);

/// \brief Draws a circle using the current rendering colour
/// \param x X position of the circle's center
/// \param y Y position of the circle's center
/// \param r Radius in pixels of the circle
/// \param lineWidth Width of the outline
/// \warning This will do nothing unless the VK2D_UNIT_GENERATION option is enabled
void vk2dRendererDrawCircleOutline(float x, float y, float r, float lineWidth);

/// \brief Draws a line using the current rendering colour
/// \param x1 First point on the line's x position
/// \param y1 First point on the line's y position
/// \param x2 Second point on the line's x position
/// \param y2 Second point on the line's y position
void vk2dRendererDrawLine(float x1, float y1, float x2, float y2);

/// \brief Renders a texture
/// \param tex Texture to draw
/// \param x x position in pixels from the top left of the window to draw it from
/// \param y y position in pixels from the top left of the window to draw it from
/// \param xscale Horizontal scale for drawing the texture (negative for flipped)
/// \param yscale Vertical scale for drawing the texture (negative for flipped)
/// \param rot Rotation to draw the texture (VK2D only uses radians)
/// \param originX X origin for rotation (in pixels)
/// \param originY Y origin for rotation (in pixels)
/// \param xInTex X position in the texture to start drawing from
/// \param yInTex Y position in the texture to start drawing from
/// \param texWidth Width of the texture to draw
/// \param texHeight Height of the texture to draw
void vk2dRendererDrawTexture(VK2DTexture tex, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float xInTex, float yInTex, float texWidth, float texHeight);

/// \brief Renders a texture
/// \param shader Shader to draw with
/// \param tex Texture to draw
/// \param x x position in pixels from the top left of the window to draw it from
/// \param y y position in pixels from the top left of the window to draw it from
/// \param xscale Horizontal scale for drawing the texture (negative for flipped)
/// \param yscale Vertical scale for drawing the texture (negative for flipped)
/// \param rot Rotation to draw the texture (VK2D only uses radians)
/// \param originX X origin for rotation (in pixels)
/// \param originY Y origin for rotation (in pixels)
/// \param xInTex X position in the texture to start drawing from
/// \param yInTex Y position in the texture to start drawing from
/// \param texWidth Width of the texture to draw
/// \param texHeight Height of the texture to draw
void vk2dRendererDrawShader(VK2DShader shader, VK2DTexture tex, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float xInTex, float yInTex, float texWidth, float texHeight);

/// \brief Renders a polygon
/// \param polygon Polygon to draw
/// \param x x position in pixels from the top left of the window to draw it from
/// \param y y position in pixels from the top left of the window to draw it from
/// \param filled Whether or not to draw the polygon filled
/// \param lineWidth Width of the lines to draw if the polygon is not failed
/// \param xscale Horizontal scale for drawing the polygon (negative for flipped)
/// \param yscale Vertical scale for drawing the polygon (negative for flipped)
/// \param rot Rotation to draw the polygon (VK2D only uses radians)
/// \param originX X origin for rotation (in pixels)
/// \param originY Y origin for rotation (in pixels)
void vk2dRendererDrawPolygon(VK2DPolygon polygon, float x, float y, bool filled, float lineWidth, float xscale, float yscale, float rot, float originX, float originY);

/************************* Shorthand for simpler drawing at no performance cost *************************/

/// \brief Draws a rectangle using the current render colour (floats all around)
#define vk2dDrawRectangle(x, y, w, h) vk2dRendererDrawRectangle(x, y, w, h, 0, 0, 0)

/// \brief Draws a rectangle outline using the current render colour (floats all around)
#define vk2dDrawRectangleOutline(x, y, w, h, lw) vk2dRendererDrawRectangleOutline(x, y, w, h, 0, 0, 0, lw)

/// \brief Draws a circle using the current render colour (floats all around)
#define vk2dDrawCircle(x, y, r) vk2dRendererDrawCircle(x, y, r)

/// \brief Draws a circle outline using the current render colour (floats all around)
#define vk2dDrawCircleOutline(x, y, r, w) vk2dRendererDrawCircleOutline(x, y, r, w)

/// \brief Draws a line using the current render colour (floats)
#define vk2dDrawLine(x1, y1, x2, y2) vk2dRendererDrawLine(x1, y1, x2, y2)

/// \brief Draws a texture with a shader (floats)
#define vk2dDrawShader(shader, texture, x, y) vk2dRendererDrawShader(shader, texture, x, y, 1, 1, 0, 0, 0, 0, 0, texture->img->width, texture->img->height)

/// \brief Draws a texture (x and y should be floats)
#define vk2dDrawTexture(texture, x, y) vk2dRendererDrawTexture(texture, x, y, 1, 1, 0, 0, 0, 0, 0, texture->img->width, texture->img->height)

/// \brief Draws a texture in detail (floats)
#define vk2dDrawTextureExt(texture, x, y, xscale, yscale, rot, originX, originY) vk2dRendererDrawTexture(texture, x, y, xscale, yscale, rot, originX, originY, 0, 0, texture->img->width, texture->img->height)

/// \brief Draws a part of a texture (x, y, xpos, ypos, w, h should be floats)
#define vk2dDrawTexturePart(texture, x, y, xPos, yPos, w, h) vk2dRendererDrawTexture(texture, x, y, 1, 1, 0, 0, 0, xPos, yPos, w, h)

/// \brief Draws a polygon's outline (x, y, and width should be floats)
#define vk2dDrawPolygonOutline(polygon, x, y, width) vk2dRendererDrawPolygon(polygon, x, y, false, width, 1, 1, 0, 0, 0)

/// \brief Draws a polygon (x and y should be floats)
#define vk2dDrawPolygon(polygon, x, y) vk2dRendererDrawPolygon(polygon, x, y, true, 0, 1, 1, 0, 0, 0)
