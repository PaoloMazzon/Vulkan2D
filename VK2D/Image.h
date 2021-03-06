/// \file Image.h
/// \author Paolo Mazzon
/// \brief Abstraction of VkImage to make managing them easier
#pragma once
#include <vulkan/vulkan.h>
#include "VK2D/Structs.h"
#include <SDL2/SDL.h>
#include <VulkanMemoryAllocator/src/vk_mem_alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Simple wrapper that groups image things together
struct VK2DImage {
	VkImage img;           ///< Internal image
	VkImageView view;      ///< Image view bound to the image
	VmaAllocation mem;     ///< Memory the image is stored on
	VK2DLogicalDevice dev; ///< Device this image belongs to
	uint32_t width;        ///< Width in pixels of the image
	uint32_t height;       ///< Height in pixels of the image
	VkDescriptorSet set;   ///< Descriptor set for this image
};

/// \brief Creates a Vulkan image (Users want VK2DTexture, not VK2DImage, they are different)
/// \param dev Device to create the image with
/// \param width Width of the image in pixels
/// \param height Height of the image in pixels
/// \param format Format of the image
/// \param aspectMask Aspect mask of the image
/// \param usage How the image will be used
/// \param samples MSAA level of the image (can't be more than max supported, 1 for no MSAA)
/// \return Returns the new image or NULL
VK2DImage vk2dImageCreate(VK2DLogicalDevice dev, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspectMask, VkImageUsageFlags usage, VkSampleCountFlagBits samples);

/// \brief Loads an image from the disk (generates mipmaps)
/// \param dev Device to get the memory from
/// \param filename Filename of the image
/// \return Returns a new image or NULL if it failed
/// \warning Requires a renderer be initialized to work
VK2DImage vk2dImageLoad(VK2DLogicalDevice dev, const char *filename);

/// \brief Creates an image from an SDL2 surface
/// \param dev Device to create the image with
/// \param pixels Pixels to create the image with, should be 32 bit RGBA
/// \param w Width in pixels of the image
/// \param h Height in pixels of the image
/// \return Returns a new image or NULL if it failed
VK2DImage vk2dImageFromPixels(VK2DLogicalDevice dev, void *pixels, int w, int h);

/// \brief Frees an image from memory
/// \param img Image to free
void vk2dImageFree(VK2DImage img);

#ifdef __cplusplus
};
#endif