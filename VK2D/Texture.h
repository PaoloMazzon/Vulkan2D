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
VK2DTexture vk2dTextureFrom(void *data, int size);

/// \brief Creates a texture meant as a drawing target
/// \param w Width of the texture
/// \param h Height of the texture
/// \return Returns a new texture or NULL if it failed
VK2DTexture vk2dTextureCreate(float w, float h);

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

/// \brief Frees a texture from memory
/// \param tex Texture to free
void vk2dTextureFree(VK2DTexture tex);

#ifdef __cplusplus
};
#endif
