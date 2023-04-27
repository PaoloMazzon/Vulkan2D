/// \file Renderer.h
/// \author Paolo Mazzon
/// \brief The main renderer that handles all rendering
#pragma once
#include "VK2D/Structs.h"
#include <SDL2/SDL.h>

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
void vk2dRendererDrawTexture(VK2DTexture tex, float x, float y, float xscale, float yscale, float rot, float originX,
							 float originY, float xInTex, float yInTex, float texWidth, float texHeight);

/// \brief Draws a texture many number of times using instanced data
/// \param tex Texture to draw
/// \param instances Array of `VK2DDrawInstance`s that will be used as what would be parameters to vk2dRendererDrawTexture (the data is copied)
/// \param count Number of elements in the instances array
/// \warning If `count` is greater than the `maxInstancedDraws` field of vk2dRendererGetLimits, `count` will instead be taken as `maxInstancedDraws`
///
/// This function is intended to be used to draw a large amount of a single texture at once with
/// less overhead than there would be by calling vk2dRendererDrawTexture `count` number of times.
/// Use the `VK2DInstance*` function to control things like rotation/scaling but rotation and
/// scaling are more computationally expensive so its best to avoid it when possible especially
/// when processing thousands of instances.
void vk2dRendererDrawInstanced(VK2DTexture tex, VK2DDrawInstance *instances, uint32_t count);

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
void
vk2dRendererDrawPolygon(VK2DPolygon polygon, float x, float y, bool filled, float lineWidth, float xscale, float yscale,
						float rot, float originX, float originY);

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

/// \brief Sets up an instance's data in full - this is quite heavy and only intended for initialization
/// \param instance Pointer to the instance data to set up
/// \param x X position of this instance
/// \param y Y position of this instance
/// \param xScale X scale of the instance
/// \param yScale Y scale of the instance
/// \param rot Rotation of the instance
/// \param xOrigin X origin of the instance
/// \param yOrigin Y origin of the instance
/// \param xInTex X position in the texture to begin rendering from for this instance
/// \param yInTex Y position in the texture to begin rendering from for this instance
/// \param wInTex Width of the texture to draw for this instance
/// \param hInTex Height of the texture to draw for this instance
/// \param colour Colour mod for this instance
void vk2dInstanceSet(VK2DDrawInstance *instance, float x, float y, float xScale, float yScale, float rot, float xOrigin,
					 float yOrigin, float xInTex, float yInTex, float wInTex, float hInTex, vec4 colour);

/// \brief Sets up an instance's data without the computationally expensive pieces
/// \param instance Pointer to the instance data to set up
/// \param x X position of this instance
/// \param y Y position of this instance
/// \param xInTex X position in the texture to begin rendering from for this instance
/// \param yInTex Y position in the texture to begin rendering from for this instance
/// \param wInTex Width of the texture to draw for this instance
/// \param hInTex Height of the texture to draw for this instance
/// \param colour Colour mod for this instance
void vk2dInstanceSetFast(VK2DDrawInstance *instance, float x, float y, float xInTex, float yInTex, float wInTex,
						 float hInTex, vec4 colour);

/// \brief Updates an instance's positional data
/// \param instance Pointer to the instance data to set up
/// \param x X position of this instance
/// \param y Y position of this instance
/// \param xScale X scale of the instance
/// \param yScale Y scale of the instance
/// \param rot Rotation of the instance
/// \param xOrigin X origin of the instance
/// \param yOrigin Y origin of the instance
void
vk2dInstanceUpdate(VK2DDrawInstance *instance, float x, float y, float xScale, float yScale, float rot, float xOrigin,
				   float yOrigin);

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
/// \param assets Array of VK2DAssetLoad structs that specify each asset you wish to load
/// \param count Number of VK2DAssetLoad structs in the array
/// \warning Pointers allocated this way are not guaranteed to be valid until after vk2dWaitAssets
///
/// This function will load each asset in the assets list in another thread in the background
/// so you may do other things to prepare your application. In essence its a non-blocking way
/// to load your resources.
void vk2dQueueAssetLoad(VK2DAssetLoad *assets, uint32_t count);

/// \brief Waits until all of the assets provided to vk2dQueueAssetLoad have been loaded
///
/// Typically you would use vk2dQueueAssetLoad after you initialize VK2D, then do your other
/// things to initialize your program, then call this once you need to start using your
/// resources to make sure they're available in time.
void vk2dWaitAssets();

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