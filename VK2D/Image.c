/// \file Image.c
/// \author Paolo Mazzon
#include "VK2D/Image.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/PhysicalDevice.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include "VK2D/Buffer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "VK2D/stb_image.h"
#include "VK2D/Renderer.h"
#include <malloc.h>

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
Uint32 rmask = 0xff000000;
Uint32 gmask = 0x00ff0000;
Uint32 bmask = 0x0000ff00;
Uint32 amask = 0x000000ff;
#else
Uint32 rmask = 0x000000ff;
Uint32 gmask = 0x0000ff00;
Uint32 bmask = 0x00ff0000;
Uint32 amask = 0xff000000;
#endif

// Internal functions

static void _vk2dImageCopyBufferToImage(VK2DLogicalDevice dev, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = vk2dLogicalDeviceGetSingleUseBuffer(dev);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
	);

	vk2dLogicalDeviceSubmitSingleBuffer(dev, commandBuffer);
}

void _vk2dImageTransitionImageLayout(VK2DLogicalDevice dev, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer buffer = vk2dLogicalDeviceGetSingleUseBuffer(dev);
	VkPipelineStageFlags sourceStage = 0;
	VkPipelineStageFlags destinationStage = 0;

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else {
		vk2dLogMessage("Unsupported image transition");
		vk2dErrorCheck(-1);
	}

	vkCmdPipelineBarrier(
			buffer,
			sourceStage, destinationStage,
			0,
			0, VK_NULL_HANDLE,
			0, VK_NULL_HANDLE,
			1, &barrier
	);

	vk2dLogicalDeviceSubmitSingleBuffer(dev, buffer);
}

// End of internal functions

VK2DImage vk2dImageCreate(VK2DLogicalDevice dev, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspectMask, VkImageUsageFlags usage, VkSampleCountFlagBits samples) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();

	VK2DImage out = malloc(sizeof(struct VK2DImage));

	if (vk2dPointerCheck(out)) {
		out->dev = dev;
		out->width = width;
		out->height = height;
		out->set = VK_NULL_HANDLE;
		VkImageCreateInfo imageCreateInfo = vk2dInitImageCreateInfo(width, height, format, usage, 1, samples);
		VmaAllocationCreateInfo allocationCreateInfo = {};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		vk2dErrorCheck(vmaCreateImage(gRenderer->vma, &imageCreateInfo, &allocationCreateInfo, &out->img, &out->mem, VK_NULL_HANDLE));

		// Create the image view
		VkImageViewCreateInfo imageViewCreateInfo = vk2dInitImageViewCreateInfo(out->img, format, aspectMask, 1);
		vk2dErrorCheck(vkCreateImageView(dev->dev, &imageViewCreateInfo, NULL, &out->view));
	} else {
		free(out);
		out = NULL;
	}

	return out;
}

VK2DImage vk2dImageLoad(VK2DLogicalDevice dev, const char *filename) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	VK2DImage out = NULL;
	VK2DBuffer stage;
	int texWidth, texHeight, texChannels;
	unsigned char* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (pixels == NULL) { // Print out filename if the image couldn't be loaded
		vk2dLogMessage("Failed to load image \"%s\"", filename);
	}

	if (vk2dPointerCheck(pixels)) {
		stage = vk2dBufferCreate(dev, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
								 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void *data;
		vmaMapMemory(gRenderer->vma, stage->mem, &data);
		memcpy(data, pixels, imageSize);
		vmaUnmapMemory(gRenderer->vma, stage->mem);

		stbi_image_free(pixels);

		out = vk2dImageCreate(dev, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT,
							  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1);


		if (vk2dPointerCheck(out)) {
			_vk2dImageTransitionImageLayout(dev, out->img, VK_IMAGE_LAYOUT_UNDEFINED,
											VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			_vk2dImageCopyBufferToImage(dev, stage->buf, out->img, texWidth, texHeight);
			_vk2dImageTransitionImageLayout(dev, out->img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		vk2dBufferFree(stage);
	}

	return out;
}

VK2DImage vk2dImageFromPixels(VK2DLogicalDevice dev, void *pixels, int w, int h) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	VK2DImage out = NULL;
	VK2DBuffer stage;

	if (vk2dPointerCheck(pixels)) {
		stage = vk2dBufferCreate(dev, w * h * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
								 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void *data;
		vmaMapMemory(gRenderer->vma, stage->mem, &data);
		memcpy(data, pixels, w * h * 4);
		vmaUnmapMemory(gRenderer->vma, stage->mem);

		out = vk2dImageCreate(dev, w, h, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT,
							  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1);


		if (vk2dPointerCheck(out)) {
			_vk2dImageTransitionImageLayout(dev, out->img, VK_IMAGE_LAYOUT_UNDEFINED,
											VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			_vk2dImageCopyBufferToImage(dev, stage->buf, out->img, w, h);
			_vk2dImageTransitionImageLayout(dev, out->img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		vk2dBufferFree(stage);
	}

	return out;
}

void vk2dImageFree(VK2DImage img) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (img != NULL) {
		vkDestroyImageView(img->dev->dev, img->view, VK_NULL_HANDLE);
		vmaDestroyImage(gRenderer->vma, img->img, img->mem);
		free(img);
	}
}