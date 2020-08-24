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
#include <malloc.h>

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

static void _vk2dImageTransitionImageLayout(VK2DLogicalDevice dev, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
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
	barrier.subresourceRange.levelCount = mipLevels;
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
	} else {
		vk2dLogMessage("!!!Unsupported image transition!!!");
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

static void _vk2dImageGenerateMipmaps(VK2DLogicalDevice dev, VkImage image, VkFormat imageFormat, uint32_t width, uint32_t height, uint32_t mipLevels) {
	// We first make sure the platform actually supports blitting things
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(dev->pd->dev, imageFormat, &formatProperties);
	vk2dErrorCheck((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) ? VK_SUCCESS : -1);

	VkCommandBuffer commandBuffer = vk2dLogicalDeviceGetSingleUseBuffer(dev);
	int32_t mipW = width;
	int32_t mipH = height;
	uint32_t i;

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	for (i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							 0, VK_NULL_HANDLE,
							 0, VK_NULL_HANDLE,
							 1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0].x = 0;
		blit.srcOffsets[0].y = 0;
		blit.srcOffsets[0].z = 0;
		blit.srcOffsets[1].x = mipW;
		blit.srcOffsets[1].y = mipH;
		blit.srcOffsets[1].z = 1;
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0].x = 0;
		blit.dstOffsets[0].y = 0;
		blit.dstOffsets[0].z = 0;
		blit.dstOffsets[1].x = mipW > 1 ? mipW / 2 : 1;
		blit.dstOffsets[1].y = mipH > 1 ? mipH / 2 : 1;
		blit.dstOffsets[1].z = 1;
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
					   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   1, &blit,
					   VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
							 0, VK_NULL_HANDLE,
							 0, VK_NULL_HANDLE,
							 1, &barrier);

		if (mipW > 1) mipW /= 2;
		if (mipH > 1) mipH /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
						 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
						 0, VK_NULL_HANDLE,
						 0, VK_NULL_HANDLE,
						 1, &barrier);

	vk2dLogicalDeviceSubmitSingleBuffer(dev, commandBuffer);
}

// End of internal functions

VK2DImage vk2dImageCreate(VK2DLogicalDevice dev, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspectMask, VkImageUsageFlags usage, VkSampleCountFlagBits samples, uint32_t mipLevels) {
	VK2DImage out = malloc(sizeof(struct VK2DImage));
	uint32_t i;

	if (vk2dPointerCheck(out)) {
		out->dev = dev;
		out->width = width;
		out->height = height;
		out->mipCount = 1;
		VkImageCreateInfo imageCreateInfo = vk2dInitImageCreateInfo(width, height, format, usage, mipLevels, samples);
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

VK2DImage vk2dImageLoad(VK2DLogicalDevice dev, const char *filename, VkSampleCountFlagBits samples) {
	VK2DImage out;
	VK2DBuffer stage;
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	stage = vk2dBufferCreate(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dev);

	void* data;
	vkMapMemory(dev->dev, stage->mem, 0, imageSize, 0, &data);
	memcpy(data, pixels, imageSize);
	vkUnmapMemory(dev->dev, stage->mem);

	stbi_image_free(pixels);

	uint32_t mipLevels = floorf(log2f(fmax(texWidth, texHeight))) + 1;
	out = vk2dImageCreate(dev, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, samples, mipLevels);

	if (vk2dPointerCheck(out)) {
		_vk2dImageTransitionImageLayout(dev, out->img, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
		_vk2dImageCopyBufferToImage(dev, stage->buf, out->img, texWidth, texHeight);
		_vk2dImageGenerateMipmaps(dev, out->img, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
	}

	vk2dBufferFree(stage);

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