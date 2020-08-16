/// \file Image.h
/// \author Paolo Mazzon
/// \brief Abstraction of VkImage to make managing them easier
#pragma once
#include <vulkan/vulkan.h>
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Simple wrapper that groups image things together
struct VK2DImage {
	VkImage img;           ///< Internal image
	VkImageView view;      ///< Image view bound to the image
	VkDeviceMemory mem;    ///< Memory the image is stored on
	VK2DLogicalDevice dev; ///< Device this image belongs to
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

/// \brief Frees an image from memory
/// \param img Image to free
void vk2dImageFree(VK2DImage img);

#ifdef __cplusplus
};
#endif