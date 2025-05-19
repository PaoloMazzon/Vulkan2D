/// \file Renderer.h
/// \author Paolo Mazzon
/// \brief The main renderer that handles all rendering
///
/// Below is a brief technical breakdown of the renderer.
///
/// The first thing VK2D does is initialize the swapchain and its corresponding resources.
/// Of note, there are
///
///  + Three maximum frames in flight (not a direct resource but relevant to everything)
///  + Three render passes - one to start the frame, one for writing to a texture, and one to swap back to after texture writes
///  + A descriptor buffer per frame in flight
///  + A framebuffer per swapchain image
///  + Several `DescriptorController`s for different resources
///  + Many pipelines, particularly one graphics pipeline for each supported blend mode per shader
///  + A depth buffer per swapchain image
///
/// Additionally, every time the window is resized (or really for any reason, that's just the most common)
/// most of those resources are recreated, which is why on older hardware changing window size could take up
/// to a few seconds.
///
/// The user may then load/create their textures/models/etc, this is mostly boring the only noteworthy
/// point is that all textures' descriptors are stored in a global descriptor array, meaning every shader
/// has access to every texture currently loaded at once without the need for more descriptor sets. This is
/// useful for instancing and sprite batching, not to mention user convenience.
///
/// Once the user is ready to start rendering they use vk2dRendererStartFrame which does a lot of things
///
///  + If there is vsync, wait till the swapchain hands us an image, otherwise wait till the frame in flight is ready (don't worry about this)
///  + Reset and begin recording the three command buffers - copy, compute, and draw
///  + Begin the renderpass on the draw buffer, and bind the sprite-batching compute shader on the compute buffer
///  + Clear the spritebatch information to prepare for this frame
///
/// Then the user may record draw commands. Some draw commands like drawing shapes/models are simple and
/// just record a vkCmdDraw to the draw buffer, but sprite-batching and switching render targets is more complex.
///
/// Switching render pass involves a few steps:
///
///  + End the render pass
///  + Put a pipeline barrier to put the image into either shader-read-optimal mode or color-attachment mode
///  + Start a new render pass depending on if the target is now a texture or the swapchain
///
/// Also, briefly, of note is the DescriptorBuffer. The DescriptorBuffer is responsible for most VRAM memory management
/// during the frame. VK2D gives the current frame's DescriptorBuffer random shader/vertex data and it puts it in
/// VRAM pages, their size specified at the start of the program via vramPageSize. At the end of each frame it copies
/// all data it was given from RAM to VRAM and inserts a pipeline barrier on the copy buffer to block compute until
/// it finishes copying. Just know that any data that is not known ahead of time is put into this, ie, sprite batch
/// input/output, vk2dRendererDrawGeometry, shader uniforms, view matrices, ...
///
/// More interestingly, sprite batching happens automatically and consists of a list in the renderer that keeps track
/// of the current batch and any sort of texture drawing. When the current batch reaches the batch limit (as determined
/// by the vram page size), a camera is changed, pipelines are swapped, or some other things, the current batch is
/// flushed. When a batch is flushed, the batch is pushed to VRAM through the current descriptor buffer, space on the
/// descriptor buffer is reserved for the compute shader output, then the compute shader to process the batch's model
/// matrices is dispatched on the compute buffer. A draw command is also queued on the draw buffer using the compute
/// shader's output as vertex input data for the instances.
///
/// At the end of the frame:
///
///  + A pipeline barrier is inserted at the end of the copy buffer to block compute until copy is done
///  + A pipeline barrier is inserted at the end of the compute buffer to block vertex input until compute is done
///  + The command buffers are ended and submitted
///  + The swapchain is presented
///
/// This is still not super detailed, but its a good starting point to understanding whats going on.
#pragma once
#include "VK2D/Structs.h"
#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Initializes VK2D's renderer
/// \param window An SDL window created with the flag SDL_WINDOW_VULKAN
/// \param config Initial renderer configuration settings
/// \param options Renderer options, or just NULL for defaults
/// \return Returns a VK2DResult enum
///
/// GPUs are not guaranteed to support certain screen modes and msaa levels (integrated
/// gpus often don't support triple buffering, 32x msaa is not terribly common), so if
/// you request something that isn't supported, the next best thing is used in its place.
///
/// VK2DStartupOptions lets you control how some meta things in the renderer, such as
/// whether or not to enable stdout logging or enable the Vulkan validation layers. Leave this
/// null for options that would generally be fine for most things.
///
/// The following are default values for VK2DStartupOptions if none are provided:
///
/// `enableDebug` defaults to `false`
/// `stdoutLogging` defaults to `true`
/// `quitOnError` defaults to `true`
/// `errorFile` defaults to `"vk2derror.txt"`
/// `loadCustomShaders` defaults to `false`
/// `vramPageSize` defaults to `256 * 1000`, setting this to 0 also uses `256 * 1000`
/// `maxTextures` defaults to 10000, setting this to 0 also uses 10000.
///
VK2DResult vk2dRendererInit(SDL_Window *window, VK2DRendererConfig config, VK2DStartupOptions *options);

/// \brief Waits until current GPU tasks are done before moving on
///
/// Make sure you call this before freeing your assets in case they're still being used.
void vk2dRendererWait();

/// \brief Frees resources used by the renderer
void vk2dRendererQuit();

/// \brief Gets the internal renderer's pointer
/// \return Returns the internal VK2DRenderer pointer (can be NULL)
/// \warning Probably don't use this.
VK2DRenderer vk2dRendererGetPointer();

/// \brief Gets the current user configuration of the renderer
/// \return Returns the structure containing the config information
///
/// This returns what is actually being used in the renderer and not what you may
/// have requested; for example, if you requested 16x msaa but the host only supports
/// up to 8x msaa, this function will return a configuration with 8x msaa.
VK2DRendererConfig vk2dRendererGetConfig();

/// \brief Resets the renderer with a new configuration
/// \param config New render user configuration to use
///
/// Changes take effect generally at the end of the frame.
void vk2dRendererSetConfig(VK2DRendererConfig config);

/// \brief Returns the amount of VRAM currently in use and free
/// \param inUse Pointer to a float that will be provided with the total amount of VRAM in use, in MiB
/// \param total Pointer to a float that will be provided with the total amount of VRAM on the system, in MiB
///
/// \warning If vk2dRendererGetLimits().supportsVRAMUsage is false, these numbers are (far) less accurate.
/// \warning These are estimates even if vk2dRendererGetLimits().supportsVRAMUsage is true.
/// Do not assume that you will be able to allocate memory up to total without issue.
///
/// This is mostly for debug purposes. If vk2dRendererGetLimits().supportsVRAMUsage is false, this number is
/// significantly less accurate. This number also may not be exactly as you expect because, for example, if you
/// loaded a couple images and shader and expected them to only total to ~50MiB, it may show as more because
/// this number may be including Vulkan objects that also live in VRAM like pipelines or render passes.
void vk2dRendererGetVRAMUsage(float *inUse, float *total);

/// \brief Forces the renderer to rebuild itself (VK2D does this automatically)
///
/// This is automatically done when Vulkan detects the window is no longer suitable,
/// but this is still available to do manually if you so desire. Generally there is
/// no reason to use this.
void vk2dRendererResetSwapchain();

/// \brief Sets up the renderer to prepare for drawing
/// \param clearColour Colour to clear the screen to
/// \warning Camera updates are applied in this function and any camera updates called after this
/// function will not be applied until the next time this function is called.
void vk2dRendererStartFrame(const vec4 clearColour);

/// \brief Completes the end-of-frame drawing tasks
/// \return Generally returns VK2D_SUCCESS, will return VK2D_RESET_SWAPCHAIN when the swapchain is reset
VK2DResult vk2dRendererEndFrame();

/// \brief Returns the logical device being used by the renderer
/// \return Returns the current logical device
VK2DLogicalDevice vk2dRendererGetDevice();

/// \brief Changes the render target to a texture or the screen
/// \param target Target texture to switch to or VK2D_TARGET_SCREEN for the screen
/// \warning This is mildly taxing on the GPU
///
/// Switches drawing operations so that they will be performed on the specified target.
/// Target must be either a texture that was created for rendering or VK2D_TARGET_SCREEN.
void vk2dRendererSetTarget(VK2DTexture target);

/// \brief Sets the rendering blend mode (does nothing if VK2D_GENERATE_BLEND_MODES is disabled)
/// \param blendMode Blend mode to use for drawing
void vk2dRendererSetBlendMode(VK2DBlendMode blendMode);

/// \brief Gets the current blend mode (will return whatever was set with vk2dRendererSetBlendMode regardless of whether blend modes are enabled or not)
/// \return Returns the current blend mode
VK2DBlendMode vk2dRendererGetBlendMode();

/// \brief Returns the handle to the Nuklear context
/// \return Returns the handle to the Nuklear context
void *vk2dGetNuklearCtx();

/// \brief Call this at the beginning of your event loop
void vk2dBeginEvents();

/// \brief Call this at the end of your event loop
void vk2dEndEvents();

/// \brief Handles SDL events that might pertain to VK2D
/// \note As of right now, this is only required if you intend to use Nuklear integration.
void vk2dProcessEvent(SDL_Event *e);

/// \brief Sets the current colour modifier (Colour all pixels are blended with)
/// \param mod Colour mod to make current
void vk2dRendererSetColourMod(const vec4 mod);

/// \brief Gets the current colour modifier
/// \param dst Destination vector to place the current colour mod into
///
/// The vec4 is treated as an RGBA array
void vk2dRendererGetColourMod(vec4 dst);

/// \brief Allows you to enable or disable the use of the renderer's camera when drawing to textures
/// \param useCameraOnTextures See below
///
/// If enabled, your cameras will be used when rendering to textures. If disabled, all drawing
/// to the texture will be done in texture space; ie, drawing at pos (20, 20) means 20
/// pixels to the right in the texture and 20 pixels down in the texture from the origin which
/// is the top-left corner just like the screen. You will often want to enable this so you
/// may draw your game to a texture.
void vk2dRendererSetTextureCamera(bool useCameraOnTextures);

/// \brief Gets the average amount of time frames are taking to process from the end of vk2dRendererEndFrame to the end of the next vk2dRendererEndFrame
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

/// \brief Clears the current render target to the current renderer colour
void vk2dRendererClear();

/// \brief Clears the content so that every pixel in the target is set to be complete transparent (useful for new texture targets)
void vk2dRendererEmpty();

/// \brief Returns the limits of the renderer on the current host
/// \return Returns a struct containing all limit information
VK2DRendererLimits vk2dRendererGetLimits();

/// \brief Draws a rectangle using the current rendering colour
/// \param x X position to draw the rectangle
/// \param y Y position to draw the rectangle
/// \param w Width of the rectangle
/// \param h Height of the rectangle
/// \param r Rotation of the rectangle
/// \param ox X origin of rotation of the rectangle (in percentage)
/// \param oy Y origin of rotation of the rectangle (in percentage)
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
void vk2dRendererDrawRectangleOutline(float x, float y, float w, float h, float r, float ox, float oy, float lineWidth);

/// \brief Draws a circle using the current rendering colour
/// \param x X position of the circle's center
/// \param y Y position of the circle's center
/// \param r Radius in pixels of the circle
void vk2dRendererDrawCircle(float x, float y, float r);

/// \brief Draws a circle using the current rendering colour
/// \param x X position of the circle's center
/// \param y Y position of the circle's center
/// \param r Radius in pixels of the circle
/// \param lineWidth Width of the outline
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
void vk2dRendererDrawTexture(VK2DTexture tex, float x, float y, float xscale, float yscale, float rot, float originX,
							 float originY, float xInTex, float yInTex, float texWidth, float texHeight);

/// \brief Adds a user-provided draw batch to the current draw batch, flushing if necessary
/// \param commands Array of draw commands
/// \param count Number of draw commands in the array
///
/// Allows users to maintain their own sprite-batches, possibly across threads or to not have
/// to dance around VK2D's automatic flushing. That said, this function is not thread-safe,
/// you may build your draw command list on any thread you like but this function must be
/// called from the same thread VK2D was created on.
void vk2dRendererAddBatch(VK2DDrawCommand *commands, uint32_t count);

/// \brief Renders a texture
/// \param shader Shader to draw with
/// \param data Uniform buffer data the shader expects; should be the size specified when the shader was created or NULL if a size of 0 was given
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
void
vk2dRendererDrawShader(VK2DShader shader, void *data, VK2DTexture tex, float x, float y, float xscale, float yscale,
					   float rot, float originX, float originY, float xInTex, float yInTex, float texWidth,
					   float texHeight);

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
/// \warning If filled is true the polygon must be triangulated.
void vk2dRendererDrawPolygon(VK2DPolygon polygon, float x, float y, bool filled, float lineWidth, float xscale, float yscale, float rot, float originX, float originY);

/// \brief Draws arbitrary geometry without needing to pre-allocate a polygon
/// \param vertices Vertices to draw
/// \param count Number of vertices in the list
/// \param x x position in pixels from the top left of the window to draw it from
/// \param y y position in pixels from the top left of the window to draw it from
/// \param filled Whether or not to draw the polygon filled
/// \param lineWidth Width of the lines to draw if the polygon is not failed
/// \param xscale Horizontal scale for drawing the polygon (negative for flipped)
/// \param yscale Vertical scale for drawing the polygon (negative for flipped)
/// \param rot Rotation to draw the polygon (VK2D only uses radians)
/// \param originX X origin for rotation (in pixels)
/// \param originY Y origin for rotation (in pixels)
/// \warning If filled is true the polygon must be triangulated.
///
/// This function works by putting the geometry into a vram page before drawing,
/// which means its limited by vk2dRendererGetLimits().maxGeometryVertices.
void vk2dRendererDrawGeometry(VK2DVertexColour *vertices, int count, float x, float y, bool filled, float lineWidth, float xscale, float yscale, float rot, float originX, float originY);

/// \brief Draws a shadow environment
/// \param shadowEnvironment Shadows to draw
/// \param colour Colour of the shadows
/// \param lightSource Light source position
void vk2dRendererDrawShadows(VK2DShadowEnvironment shadowEnvironment, vec4 colour, vec2 lightSource);

/// \brief Renders a 3D model
/// \param model Model to render
/// \param x x position to draw at
/// \param y y position to draw at
/// \param z z position to draw at
/// \param xscale Scale that will be applied to the x-plane of the model
/// \param yscale Scale that will be applied to the y-plane of the model
/// \param zscale Scale that will be applied to the z-plane of the model
/// \param rot Rotation of the model
/// \param zrot Z axis to rotate on
/// \param originX x origin to rotate around
/// \param originY y origin to rotate around
/// \param originZ z origin to rotate around
/// \warning This function will only render to 3D-enabled cameras (which you must set up yourself) and if there are
/// none available this function will simply do nothing.
///
/// VK2D is not optimized for 3D rendering and this is more of a fun option you can use to add cool
/// effects to your 2D games; the save points in Castlevania SoTN are a good example. VK2D provides
/// a depth buffer by default.
void
vk2dRendererDrawModel(VK2DModel model, float x, float y, float z, float xscale, float yscale, float zscale, float rot,
					  vec3 axis, float originX, float originY, float originZ);

/// \brief Renders a 3D model as a wireframe
/// \param model Model to render
/// \param x x position to draw at
/// \param y y position to draw at
/// \param z z position to draw at
/// \param xscale Scale that will be applied to the x-plane of the model
/// \param yscale Scale that will be applied to the y-plane of the model
/// \param zscale Scale that will be applied to the z-plane of the model
/// \param rot Rotation of the model
/// \param zrot Z axis to rotate on
/// \param originX x origin to rotate around
/// \param originY y origin to rotate around
/// \param originZ z origin to rotate around
/// \param lineWidth Width of the "wires"
/// \warning This function will only render to 3D-enabled cameras (which you must set up yourself) and if there are
/// none available this function will simply do nothing.
/// \warning The wireframe may not look as you expect because models are triangulated
void vk2dRendererDrawWireframe(VK2DModel model, float x, float y, float z, float xscale, float yscale, float zscale,
							   float rot, vec3 axis, float originX, float originY, float originZ, float lineWidth);

/// \brief Converts a Hex colour into a vec4 normalized colour
/// \param dst Destination vec4 to place the colour into
/// \param hex Hex colour as a string, alpha is assumed to be 100%
void vk2dColourHex(vec4 dst, const char *hex);

/// \brief Converts a Int colour into a vec4 normalized colour
/// \param dst Destination vec4 to place the colour into
/// \param colour Int encoded as RGBA, one byte each
void vk2dColourInt(vec4 dst, uint32_t colour);

/// \brief Converts a RGBA colour into a vec4 normalized colour
/// \param dst Destination vec4 to place the colour into
/// \param r Red component, from 0-255
/// \param g Green component, from 0-255
/// \param b Blue component, from 0-255
/// \param a Alpha component, from 0-255
void vk2dColourRGBA(vec4 dst, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/// \brief Forces the renderer to flush the current sprite batch
/// The renderer automatically does this in a few conditions but you
/// can force it to flush if you need to for whatever reason.
///
/// The renderer automatically flushes under these circumstances:
///
///  + The current pipeline is switched (user goes from drawing textures to 3D models or shapes, blend mode is switched, ...)
///  + vk2dRendererGetLimits().maxInstancedDraws is reached in the current batch
///  + Render target changes
///  + End of the frame
///  + Camera lock is changed
///  + A camera is enabled/disabled
void vk2dRendererFlushSpriteBatch();

/// \brief Returns a random number between min and max, thread safe
/// \param min Minimum value that can be returned
/// \param max Maximum value that can be returns
/// \return Returns a random number between min and max
///
/// This function is thread safe and decently random. It is however not
/// secure and only intended for testing/games. The renderer automatically
/// initializes the seed to system time.
float vk2dRandom(float min, float max);

/// \brief Loads a number of assets in a background thread
/// \param assets Array of VK2DAssetLoad structs that specify each asset you wish to load. The list is copied but not the data/strings inside it.
/// \param count Number of VK2DAssetLoad structs in the array
/// \warning Pointers allocated this way are not guaranteed to be valid until after vk2dAssetsWait
/// \warning You may not call this again until vk2dAssetsWait is called
/// \warning If vk2dRendererGetLimits().supportsMultiThreadLoading is false this will perform the asset load on the main thread (and be blocking)
///
/// This function will load each asset in the assets list in another thread in the background
/// so you may do other things to prepare your application. In essence its a non-blocking way
/// to load your resources. If `vk2dRendererGetLimits().supportsMultiThreadLoading` is false
/// this function will still load all of the specified assets, but it will be done on the main
/// thread instead which will be blocking.
///
/// \warning This is currently unsupported until it is re-implemented in a more cross-platform manner
void vk2dAssetsLoad(VK2DAssetLoad *assets, uint32_t count);

/// \brief Waits until all of the assets provided to vk2dAssetsLoad have been loaded
/// \warning If vk2dRendererGetLimits().supportsMultiThreadLoading is false this does nothing
///
/// Typically you would use vk2dAssetsLoad after you initialize VK2D, then do your other
/// things to initialize your program, then call this once you need to start using your
/// resources to make sure they're available in time.
void vk2dAssetsWait();

/// \brief Returns the loading status as a percentage from 0-1
/// \return Returns a status where 0 is nothing is loaded and 1 is everything is loaded
/// \warning If vk2dRendererGetLimits().supportsMultiThreadLoading is false this will return 1
float vk2dAssetsLoadStatus();

/// \brief Returns true if the assets thread is done loading assets
/// \warning If vk2dRendererGetLimits().supportsMultiThreadLoading is false this returns true
bool vk2dAssetsLoadComplete();

/// \brief Uses the same asset list you passed to vk2dAssetsLoad to free all the assets in one call
/// \param assets Assets to free
/// \param count Number of assets in the array
void vk2dAssetsFree(VK2DAssetLoad *assets, uint32_t count);

/// \brief Sets up a VK2DAssetLoad array entry for a TextureFile entry
/// \param array VK2DAssetLoad array that will be written to
/// \param index Index in the array to write to
/// \param filename Texture's filename
/// \param outVar Variable that, once the asset is loaded, will contain the loaded asset
void vk2dAssetsSetTextureFile(VK2DAssetLoad *array, int index, const char *filename, VK2DTexture *outVar);

/// \brief Sets up a VK2DAssetLoad array entry for a TextureMemory entry
/// \param array VK2DAssetLoad array that will be written to
/// \param index Index in the array to write to
/// \param buffer Texture's file buffer
/// \param size Size of the file buffer in bytes
/// \param outVar Variable that, once the asset is loaded, will contain the loaded asset
void vk2dAssetsSetTextureMemory(VK2DAssetLoad *array, int index, void *buffer, int size, VK2DTexture *outVar);

/// \brief Sets up a VK2DAssetLoad array entry for a ModelFile entry
/// \param array VK2DAssetLoad array that will be written to
/// \param index Index in the array to write to
/// \param filename Model's filename
/// \param texture Texture to bind to the model
/// \param outVar Variable that, once the asset is loaded, will contain the loaded asset
void vk2dAssetsSetModelFile(VK2DAssetLoad *array, int index, const char *filename, VK2DTexture *texture, VK2DModel *outVar);

/// \brief Sets up a VK2DAssetLoad array entry for a ModelMemory entry
/// \param array VK2DAssetLoad array that will be written to
/// \param index Index in the array to write to
/// \param buffer Model's file buffer
/// \param size Size of the file buffer in bytes
/// \param texture Texture to bind to the model
/// \param outVar Variable that, once the asset is loaded, will contain the loaded asset
void vk2dAssetsSetModelMemory(VK2DAssetLoad *array, int index, void *buffer, int size, VK2DTexture *texture, VK2DModel *outVar);

/// \brief Sets up a VK2DAssetLoad array entry for a ShaderFile entry
/// \param array VK2DAssetLoad array that will be written to
/// \param index Index in the array to write to
/// \param vertexFilename Filename of the vertex shader
/// \param fragmentFilename Filename of the fragment shader
/// \param uniformBufferSize Uniform buffer size
/// \param outVar Variable that, once the asset is loaded, will contain the loaded asset
void vk2dAssetsSetShaderFile(VK2DAssetLoad *array, int index, const char *vertexFilename, const char *fragmentFilename, uint32_t uniformBufferSize, VK2DShader *outVar);

/// \brief Sets up a VK2DAssetLoad array entry for a ShaderMemory entry
/// \param array VK2DAssetLoad array that will be written to
/// \param index Index in the array to write to
/// \param vertexBuffer File buffer for the vertex shader
/// \param vertexBufferSize Size of the vertex buffer in bytes
/// \param fragmentBuffer File buffer for the fragment shader
/// \param fragmentBufferSize Size of the fragment buffer in bytes
/// \param uniformBufferSize Uniform buffer size
/// \param outVar Variable that, once the asset is loaded, will contain the loaded asset
void vk2dAssetsSetShaderMemory(VK2DAssetLoad *array, int index, void *vertexBuffer, int vertexBufferSize, void *fragmentBuffer, int fragmentBufferSize, uint32_t uniformBufferSize, VK2DShader *outVar);

/// \brief Combines busy loops and SDL_Delay for a more accurate sleep function
/// \param seconds Time in seconds to sleep - values equal or less than 0 do nothing
///
/// This is a much more accurate sleep function that something like SDL_Delay without taxing
/// the CPU as much as a simple busy loop.
void vk2dSleep(double seconds);

/// \brief Gets the renderer's current status code.
/// \return Returns the most recent status code
VK2DStatus vk2dStatus();

/// \brief Returns true if the current status code should be considered fatal
/// \returns Returns if the current renderer status is fatal.
///
/// If you have `VK2DStartupOptions.quitOnError` set to true (which is the default option)
/// a fatal status would have already crashed the program before you can check this. You
/// may, however, choose to disable crashing on an error and quit gracefully on your own in
/// which case this is very helpful. Some status are not considered fatal, like if a texture
/// file is missing so you should use this to check for critical errors and not vk2dStatus().
bool vk2dStatusFatal();

/// \brief Returns the current renderer status message, generally use this if vk2dGetStatus() returns something other than
/// VK2D_STATUS_NONE
/// \return Returns a message describing the most recent status code.
///
/// Usually the status code is descriptive enough to figure out what happened but this can be helpful for a user facing
/// error message. This is automatically put to a file if `VK2DStartupOptions.errorFile` is a valid filename.
const char *vk2dStatusMessage();

/// \brief Returns a string detailing information about the host machine
/// \returns A string detailing information about the host machine
/// \warning Will simply return an empty string before VK2D is initialized
const char *vk2dHostInformation();

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
#define vk2dDrawShader(shader, data, texture, x, y) vk2dRendererDrawShader(shader, data, texture, x, y, 1, 1, 0, 0, 0, 0, 0, vk2dTextureWidth(texture), vk2dTextureHeight(texture))

/// \brief Draws a texture (x and y should be floats)
#define vk2dDrawTexture(texture, x, y) vk2dRendererDrawTexture(texture, x, y, 1, 1, 0, 0, 0, 0, 0, vk2dTextureWidth(texture), vk2dTextureHeight(texture))

/// \brief Draws a texture in detail (floats)
#define vk2dDrawTextureExt(texture, x, y, xscale, yscale, rot, originX, originY) vk2dRendererDrawTexture(texture, x, y, xscale, yscale, rot, originX, originY, 0, 0, vk2dTextureWidth(texture), vk2dTextureHeight(texture))

/// \brief Draws a part of a texture (x, y, xpos, ypos, w, h should be floats)
#define vk2dDrawTexturePart(texture, x, y, xPos, yPos, w, h) vk2dRendererDrawTexture(texture, x, y, 1, 1, 0, 0, 0, xPos, yPos, w, h)

/// \brief Draws a polygon's outline (x, y, and width should be floats)
#define vk2dDrawPolygonOutline(polygon, x, y, width) vk2dRendererDrawPolygon(polygon, x, y, false, width, 1, 1, 0, 0, 0)

/// \brief Draws a polygon (x and y should be floats)
#define vk2dDrawPolygon(polygon, x, y) vk2dRendererDrawPolygon(polygon, x, y, true, 0, 1, 1, 0, 0, 0)

/// \brief Draws a model (all floats)
#define vk2dDrawModel(model, x, y, z) vk2dRendererDrawModel(model, x, y, z, 1, 1, 1, 0, 1, 0, 0, 0)

/// \brief Draws a model with some extra parts (all floats)
#define vk2dDrawModelExt(model, x, y, z, xscale, yscale, zscale) vk2dRendererDrawModel(model, x, y, z, xscale, yscale, zscale, 0, 1, 0, 0, 0)

/// \brief Draws a wireframe (all floats)
#define vk2dDrawWireframe(model, x, y, z) vk2dRendererDrawWireframe(model, x, y, z, 1, 1, 1, 0, 1, 0, 0, 0, 1)

/// \brief Draws a wireframe with some extra parts (all floats)
#define vk2dDrawWireframeExt(model, x, y, z, xscale, yscale, zscale, lineWidth) vk2dRendererDrawModel(model, x, y, z, xscale, yscale, zscale, 0, 1, 0, 0, 0, lineWidth)

#ifdef __cplusplus
}
#endif