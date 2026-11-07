/// \file Texture.h
/// \author Paolo Mazzon
/// \brief Makes managing textures samplers and off-screen rendering simpler
#pragma once
#include <vulkan/vulkan.h>
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Creates a texture from an image
/// \param image Image to use
/// \return Returns a new texture or NULL if it failed
/// \warning Textures created with this function are NOT valid render targets
VK2DTexture vk2dTextureLoadFromImage(VK2DImage image);

/// \brief Loads a texture from a file (png, bmp, jpg, tiff)
/// \param filename File to load
/// \return Returns a new texture or NULL if it failed
/// \warning Textures created with this function are NOT valid render targets
VK2DTexture vk2dTextureLoad(const char *filename);

/// \brief Same as vk2dTextureLoad but it uses a byte buffer instead of pulling from a file
/// \param data Pointer to the image data, either png, bmp, jpg, or tiff
/// \param size Size in bytes of the data buffer
/// \return Returns a new texture or NULL if it failed
/// \warning Textures created with this function are NOT valid render targets
VK2DTexture vk2dTextureFrom(const void *data, int size);

/// \brief Creates a texture meant as a drawing target - see `vk2dRendererSetTarget`
/// \param w Width of the texture
/// \param h Height of the texture
/// \return Returns a new texture or NULL if it failed
/// \warning If you do not completely fill the created texture (ie, with something like `vk2dRendererEmpty` or
/// `vk2dRendererClear`) before you draw this texture it ***will*** cause crashes on certain hardware.
VK2DTexture vk2dTextureCreate(float w, float h);

/// \brief Replaces all pixels of a texture
/// \param tex Texture loaded from a file, memory or an image; render targets are refused
/// \param pixels Tightly packed 8-bit RGBA pixels, width * height * 4 bytes
/// \return Returns true if the pixels were uploaded
/// \warning Same rules as vk2dImageUpdate: main thread only, waits for the device queue to go idle, and the new
/// pixels apply to every draw of the frame being recorded.
bool vk2dTextureUpdate(VK2DTexture tex, const void *pixels);

/// \brief Replaces a rectangle of pixels in a texture
/// \param tex Texture loaded from a file, memory or an image; render targets are refused
/// \param pixels Tightly packed 8-bit RGBA pixels for the rectangle, w * h * 4 bytes
/// \param x Left edge of the rectangle in pixels
/// \param y Top edge of the rectangle in pixels
/// \param w Width of the rectangle in pixels
/// \param h Height of the rectangle in pixels
/// \return Returns true if the pixels were uploaded, false if the texture is a render target or the rectangle does
/// not fit inside it
/// \warning Same rules as vk2dImageUpdate.
bool vk2dTextureUpdateRegion(VK2DTexture tex, const void *pixels, int x, int y, int w, int h);

/// \brief Gets the width in pixels of a texture
/// \param tex Texture to get the width from
/// \return Returns the width in pixels
float vk2dTextureWidth(VK2DTexture tex);

/// \brief Gets the height in pixels of a texture
/// \param tex Texture to get the height from
/// \return Returns the height in pixels
float vk2dTextureHeight(VK2DTexture tex);

/// \brief Checks if a texture was created as a target or not
/// \param tex Texture to check
/// \return Returns true if its a valid target, false otherwise
bool vk2dTextureIsTarget(VK2DTexture tex);

/// \brief Returns a texture's internal image
/// \param tex Texture to check
/// \return Returns the texture's image
VK2DImage vk2dTextureGetImage(VK2DTexture tex);

/// \brief Returns a unique ID for this texture
/// \param tex Texture to get the id of
/// \return Returns a unique uint32_t ID for this texture, used for sprite batching/user shaders
///
/// This function is thread-safe.
uint32_t vk2dTextureGetID(VK2DTexture tex);

/// \brief Frees a texture from memory
/// \param tex Texture to free
void vk2dTextureFree(VK2DTexture tex);

/// \brief Reads a texture's pixels back from the GPU
/// \param tex Texture to read, either loaded or a render target
/// \param outPixels Buffer of at least width * height * 4 bytes that receives tightly packed 8-bit RGBA pixels
/// \return Returns true if the pixels were read
/// \warning Not thread-safe, and waits for the device queue to go idle.
bool vk2dTextureCopyPixels(VK2DTexture tex, void *outPixels);

#ifdef __cplusplus
};
#endif
