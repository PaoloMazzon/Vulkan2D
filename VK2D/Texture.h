/// \file Texture.h
/// \author Paolo Mazzon
/// \brief Makes managing textures samplers and off-screen rendering simpler
#pragma once
#include <vulkan/vulkan.h>
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Takes the headache out of Vulkan textures
///
/// Only textures created with vk2dTextureCreate may be rendered to. Should you try to
/// set the render target to a texture not created with vk2dTextureCreate, you can expect
/// a segfault.
struct VK2DTexture {
	VkSampler *imgSampler; ///< Sampler to make it shader visible (pointer to the universal one made by and maintained by the renderer)
	VK2DImage img;         ///< Internal image
	VK2DImage sampledImg;  ///< Image for MSAA
	VK2DPolygon bounds;    ///< Needed to render and more so to store the texture coordinates
	VkFramebuffer fbo;     ///< Framebuffer of this texture so it can be drawn to
	VK2DBuffer ubo;        ///< UBO that will be used when drawing to this texture
};

/// \brief Creates a texture from an image
/// \param image Image to use
/// \param xInImage X position in the image the texture begins (in pixels)
/// \param yInImage Y position in the image the texture begins (in pixels)
/// \param wInImage Width of the image to draw (in pixels)
/// \param hInImage Height of the image to draw (in pixels)
/// \return Returns a new texture or NULL if it failed
/// \warning Textures are closely tied to the renderer and require the renderer be initialized to load and use them
/// \warning Textures created with this function are NOT valid render targets
///
/// The *InImage parameters specify what regions of the image the texture is to draw. This is
/// essentially just so you can have sprite sheets.
VK2DTexture vk2dTextureLoad(VK2DImage image, float xInImage, float yInImage, float wInImage, float hInImage);

/// \brief Creates a texture meant as a drawing target
/// \param dev Device to create the texture with (this isn't required in vk2dTextureLoad because the image you pass it already knows what device to use)
/// \param w Width of the texture
/// \param h Height of the texture
/// \return Returns a new texture or NULL if it failed
/// \warning Textures are closely tied to the renderer and require the renderer be initialized to load and use them
VK2DTexture vk2dTextureCreate(VK2DLogicalDevice dev, float w, float h);

/// \brief Creates a texture from an image and polygon, allowing you to specify the texture's polygon
/// \param image Image to use for the texture
/// \param poly Polygon to use with the texture (must be created with vk2dPolygonTextureCreate)
/// \return Returns a new texture
/// \warning Textures are closely tied to the renderer and require the renderer be initialized to load and use them
/// \warning The polygon will be destroyed when the class is destroyed, do not free it on your own
/// \warning Textures created with this function are NOT valid render targets
VK2DTexture vk2dTextureCreateFrom(VK2DImage image, VK2DPolygon poly);

/// \brief Frees a texture from memory
/// \param tex Texture to free
void vk2dTextureFree(VK2DTexture tex);

#ifdef __cplusplus
};
#endif
