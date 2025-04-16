/// \file Texture.c
/// \author Paolo Mazzon
#include "VK2D/Texture.h"
#include "VK2D/Image.h"
#include "VK2D/Initializers.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/Validation.h"
#include "VK2D/Polygon.h"
#include "VK2D/Renderer.h"
#include "VK2D/Buffer.h"
#include "VK2D/DescriptorControl.h"
#include "VK2D/stb_image.h"
#include "VK2D/Opaque.h"
#include "VK2D/Util.h"
#include <malloc.h>

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

VK2DTexture _vk2dTextureFromInternal(void *data, int size, bool mainThread) {
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

VK2DTexture vk2dTextureFrom(void *data, int size) {
	VK2DTexture tex = _vk2dTextureFromInternal(data, size, true);
	if (tex == NULL)
        vk2dLog("Failed to load texture from data of size %i.", size);
	else
        _vk2dTextureAddToTextureArray(tex);
	return tex;
}

VK2DTexture vk2dTextureLoad(const char *filename) {
	uint32_t size;
    VK2DTexture tex = NULL;
    void *data = _vk2dLoadFile(filename, &size);
	if (data != NULL) {
        tex = _vk2dTextureFromInternal(data, size, true);
        _vk2dTextureAddToTextureArray(tex);
        free(data);
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
		out->img = vk2dImageCreate(dev, w, h, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1);
		out->sampledImg = vk2dImageCreate(dev, w, h, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, (VkSampleCountFlagBits)renderer->config.msaa);
		out->depthBuffer = vk2dImageCreate(dev, w, h, renderer->depthBufferFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, (VkSampleCountFlagBits)renderer->config.msaa);
		_vk2dImageTransitionImageLayout(dev, out->img->img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true);
		_vk2dImageTransitionImageLayout(dev, out->sampledImg->img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true);
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
