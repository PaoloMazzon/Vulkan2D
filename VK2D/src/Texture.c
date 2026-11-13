/// \file Texture.c
/// \author Paolo Mazzon
#include <stdlib.h>
#include "VK2D/Texture.h"
#include "VK2D/Buffer.h"
#include "VK2D/DescriptorControl.h"
#include "VK2D/Image.h"
#include "VK2D/Initializers.h"
#include "VK2D/Opaque.h"
#include "VK2D/Renderer.h"
#include "VK2D/Util.h"
#include "VK2D/Validation.h"
#include "VK2D/stb_image.h"
#include "VK2D/Logger.h"
#include "VK2D/LogicalDevice.h"

static void _vk2dTextureAddToTextureArray(VK2DTexture tex) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (tex == NULL || vk2dStatusFatal())
        return;

    // Find an available slot
    int spot = -1;
    for (int i = 0; i < gRenderer->options.maxTextures; i++) {
        if (!gRenderer->textureArray[i].active) {
            spot = i;
            break;
        }
    }

    if (spot == -1) {
        vk2dRaise(VK2D_STATUS_BAD_ASSET, "Ran out of space for more textures.");
    } else {
        SDL_SetAtomicInt(&tex->descriptorIndex, spot);
        gRenderer->textureArray[spot].active = true;

        // Write the descriptor set
        VkDescriptorImageInfo imageInfo = {
                .imageView = tex->img->view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .dstArrayElement = spot,
                .pImageInfo = &imageInfo,
                .dstBinding = 2,
                .dstSet = gRenderer->texArrayDescriptorSet,
                .descriptorCount = 1,
        };
        vkUpdateDescriptorSets(gRenderer->ld->dev, 1, &write, 0, VK_NULL_HANDLE);
    }
}

VK2DTexture _vk2dTextureLoadFromImageInternal(VK2DImage image, bool mainThread) {
	VK2DTexture out = calloc(1, sizeof(struct VK2DTexture_t));
	VK2DRenderer renderer = vk2dRendererGetPointer();
	if (renderer == NULL)
	    return NULL;

	if (out != NULL) {
		out->img = image;
		_vk2dTextureAddToTextureArray(out);
	} else {
        vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate texture.");
	}

	return out;
}

VK2DTexture vk2dTextureLoadFromImage(VK2DImage image) {
	return _vk2dTextureLoadFromImageInternal(image, true);
}

VK2DTexture _vk2dTextureFromInternal(const void *data, int size, bool mainThread) {
	VK2DImage image;
	VK2DTexture out = NULL;

	int x, y, channels;
	void *pixels = stbi_load_from_memory(data, size, &x, &y, &channels, 4);
	if (pixels != NULL) {
		image = vk2dImageFromPixels(vk2dRendererGetDevice(), pixels, x, y, mainThread);
		if (image != NULL) {
			out = _vk2dTextureLoadFromImageInternal(image, mainThread);
			if (out != NULL)
				out->imgHandled = true;
			else
				vk2dImageFree(image);
		}
	} else {
        vk2dRaise(VK2D_STATUS_BAD_FORMAT, "Problem with texture image format.");
	}

	if (pixels != NULL) {
		stbi_image_free(pixels);
	}

	return out;
}

VK2DTexture vk2dTextureFrom(const void *data, int size) {
	VK2DTexture tex = _vk2dTextureFromInternal(data, size, true);
	if (tex == NULL)
        vk2dLogError("Failed to load texture from data of size %i.", size);
	return tex;
}

VK2DTexture vk2dTextureLoad(const char *filename) {
	uint32_t size;
    VK2DTexture tex = NULL;
    void *data = _vk2dLoadFile(filename, &size);
	if (data != NULL) {
        tex = _vk2dTextureFromInternal(data, size, true);
        free(data);
    } else {

    }
	return tex;
}

void _vk2dCameraUpdateUBO(VK2DUniformBufferObject *ubo, VK2DCameraSpec *camera, int index);
void _vk2dImageTransitionImageLayout(VK2DLogicalDevice dev, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, bool mainThread);
void _vk2dRendererAddTarget(VK2DTexture tex);
void _vk2dRendererRemoveTarget(VK2DTexture tex);
VK2DTexture vk2dTextureCreate(float w, float h) {
	VK2DTexture out = malloc(sizeof(struct VK2DTexture_t));
	VK2DRenderer renderer = vk2dRendererGetPointer();
	VK2DLogicalDevice dev = vk2dRendererGetDevice();

	if (renderer == NULL) {
	    return NULL;
	}

	// For the UBO
	VK2DCameraSpec cam = {
			VK2D_CAMERA_TYPE_DEFAULT,
			0,
			0,
			w,
			h,
			1,
			0,
			0,
			0,
			w,
			h
	};
	VK2DUniformBufferObject ubo = {0};
	_vk2dCameraUpdateUBO(&ubo, &cam, 0);

	if (out != NULL) {
		out->img = vk2dImageCreate(dev, w, h, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 1);
		out->sampledImg = NULL;
		if (renderer->config.msaa > 1) {
			out->sampledImg = vk2dImageCreate(dev, w, h, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, (VkSampleCountFlagBits)renderer->config.msaa);
			_vk2dImageTransitionImageLayout(dev, out->sampledImg->img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true);
		}
		out->depthBuffer = vk2dImageCreate(dev, w, h, renderer->depthBufferFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, (VkSampleCountFlagBits)renderer->config.msaa);
		_vk2dImageTransitionImageLayout(dev, out->img->img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true);
		//_vk2dImageTransitionImageLayout(dev, out->depthBuffer->img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		// Set up FBO
		const int attachCount = renderer->config.msaa > 1 ? 3 : 2;
		VkImageView attachments[3];
		if (renderer->config.msaa > 1) {
			attachments[0] = out->sampledImg->view;
			attachments[1] = out->depthBuffer->view;
			attachments[2] = out->img->view;
		} else {
			attachments[0] = out->img->view;
			attachments[1] = out->depthBuffer->view;
		}

		VkFramebufferCreateInfo framebufferCreateInfo = vk2dInitFramebufferCreateInfo(renderer->externalTargetRenderPass, w, h, attachments, attachCount);
		VkResult result = vkCreateFramebuffer(dev->dev, &framebufferCreateInfo, VK_NULL_HANDLE, &out->fbo);
		if (result != VK_SUCCESS) {
		    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create framebuffer for texture of size %.2fx%.2f, Vulkan error %i", w, h, result);
		}

		// And the UBO
		out->ubo = vk2dBufferLoad(dev, sizeof(VK2DUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &ubo, true);
		out->uboSet = vk2dDescConGetBufferSet(renderer->descConVP, out->ubo);

		_vk2dRendererAddTarget(out);
		_vk2dTextureAddToTextureArray(out);
	} else {
		vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate texture.");
	}

	return out;
}

bool vk2dTextureUpdate(VK2DTexture tex, const void *pixels) {
	if (tex == NULL || tex->img == NULL) {
		vk2dRaise(VK2D_STATUS_BAD_ASSET, "Cannot update a NULL texture.");
		return false;
	}
	return vk2dTextureUpdateRegion(tex, pixels, 0, 0, (int)tex->img->width, (int)tex->img->height);
}

bool vk2dTextureUpdateRegion(VK2DTexture tex, const void *pixels, int x, int y, int w, int h) {
	if (tex == NULL || tex->img == NULL) {
		vk2dRaise(VK2D_STATUS_BAD_ASSET, "Cannot update a NULL texture.");
		return false;
	}
	if (vk2dTextureIsTarget(tex)) {
		vk2dRaise(VK2D_STATUS_BAD_ASSET, "Cannot update the pixels of a render target texture.");
		return false;
	}
	return vk2dImageUpdate(tex->img, pixels, x, y, w, h);
}

float vk2dTextureWidth(VK2DTexture tex) {
	return tex->img->width;
}

float vk2dTextureHeight(VK2DTexture tex) {
	return tex->img->height;
}

bool vk2dTextureIsTarget(VK2DTexture tex) {
	return tex->fbo != VK_NULL_HANDLE;
}

VK2DImage vk2dTextureGetImage(VK2DTexture tex) {
	return tex->img;
}

void vk2dTextureFree(VK2DTexture tex) {
	if (tex != NULL) {
		if (tex->fbo != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(tex->img->dev->dev, tex->fbo, VK_NULL_HANDLE);
			vk2dImageFree(tex->img);
			vk2dBufferFree(tex->ubo);
			vk2dImageFree(tex->sampledImg);
			vk2dImageFree(tex->depthBuffer);
			_vk2dRendererRemoveTarget(tex);
		} else if (tex->imgHandled) {
			vk2dImageFree(tex->img);
		}
        VK2DRenderer renderer = vk2dRendererGetPointer();
		int val = SDL_GetAtomicInt(&tex->descriptorIndex);
		renderer->textureArray[val].active = false;
		free(tex);
	}
}

uint32_t vk2dTextureGetID(VK2DTexture tex) {
    uint32_t val = SDL_GetAtomicInt(&tex->descriptorIndex);
    return val;
}

bool vk2dTextureCopyPixels(VK2DTexture tex, void *outPixels) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	VK2DLogicalDevice dev = vk2dRendererGetDevice();
	if (gRenderer == NULL || dev == NULL || tex == NULL || tex->img == NULL || outPixels == NULL)
		return false;

	uint32_t w = tex->img->width;
	uint32_t h = tex->img->height;
	VkDeviceSize size = (VkDeviceSize)w * h * 4;

	VK2DBuffer stage = vk2dBufferCreate(dev, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (stage == NULL)
		return false;

	VkCommandBuffer cb = vk2dLogicalDeviceGetSingleUseBuffer(dev);
	if (cb == VK_NULL_HANDLE) {
		vk2dBufferFree(stage);
		return false;
	}

	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = tex->img->img;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);

	VkBufferImageCopy region = {0};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = w;
	region.imageExtent.height = h;
	region.imageExtent.depth = 1;

	vkCmdCopyImageToBuffer(cb, tex->img->img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stage->buf, 1, &region);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
						 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);

	vk2dLogicalDeviceSubmitSingleBuffer(dev, cb);

	void *mapped = NULL;
	VkResult res = vmaMapMemory(gRenderer->vma, stage->mem, &mapped);
	if (res == VK_SUCCESS && mapped != NULL) {
		const uint8_t *src = (const uint8_t *)mapped;
		uint8_t *dst = (uint8_t *)outPixels;
		// Render targets are BGRA, loaded textures are already RGBA
		const int red = vk2dTextureIsTarget(tex) ? 2 : 0;
		for (uint32_t i = 0; i < w * h; i++) {
			dst[i * 4 + 0] = src[i * 4 + red];
			dst[i * 4 + 1] = src[i * 4 + 1];
			dst[i * 4 + 2] = src[i * 4 + 2 - red];
			dst[i * 4 + 3] = src[i * 4 + 3];
		}
		vmaUnmapMemory(gRenderer->vma, stage->mem);
	}

	vk2dBufferFree(stage);
	return res == VK_SUCCESS;
}
