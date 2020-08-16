/// \file Image.c
/// \author Paolo Mazzon
#include "VK2D/Image.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/PhysicalDevice.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include <malloc.h>

VK2DImage vk2dImageCreate(VK2DLogicalDevice dev, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspectMask, VkImageUsageFlags usage, VkSampleCountFlagBits samples) {
	VK2DImage out = malloc(sizeof(struct VK2DImage));
	uint32_t i;

	if (vk2dPointerCheck(out)) {
		out->dev = dev;
		VkImageCreateInfo imageCreateInfo = vk2dInitImageCreateInfo(width, height, format, usage, 1, samples);
		vk2dErrorCheck(vkCreateImage(dev->dev, &imageCreateInfo, NULL, &out->img));

		// Get memory requirement
		VkMemoryRequirements imageMemoryRequirements;
		uint32_t memoryIndex = UINT32_MAX;
		vkGetImageMemoryRequirements(dev->dev, out->img, &imageMemoryRequirements);
		for (i = 0; i < dev->pd->mem.memoryTypeCount && memoryIndex == UINT32_MAX; i++)
			if (imageMemoryRequirements.memoryTypeBits & (1 << i))
				if ((dev->pd->mem.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
					memoryIndex = i;

		if (vk2dErrorInline(memoryIndex == UINT32_MAX ? -1 : VK_SUCCESS)) {
			// Allocate memory
			VkMemoryAllocateInfo memoryAllocateInfo = vk2dInitMemoryAllocateInfo(imageMemoryRequirements.size,
																			   memoryIndex);
			vk2dErrorCheck(vkAllocateMemory(dev->dev, &memoryAllocateInfo, NULL, &out->mem));
			vk2dErrorCheck(vkBindImageMemory(dev->dev, out->img, out->mem, 0));

			// Create the image view
			VkImageViewCreateInfo imageViewCreateInfo = vk2dInitImageViewCreateInfo(out->img, format, aspectMask, 1);
			vk2dErrorCheck(vkCreateImageView(dev->dev, &imageViewCreateInfo, NULL, &out->view));
		} else {
			free(out);
			out = NULL;
		}
	}

	return out;
}

void vk2dImageFree(VK2DImage img) {
	if (img != NULL) {
		vkFreeMemory(img->dev->dev, img->mem, VK_NULL_HANDLE);
		vkDestroyImageView(img->dev->dev, img->view, VK_NULL_HANDLE);
		vkDestroyImage(img->dev->dev, img->img, VK_NULL_HANDLE);
		free(img);
	}
}