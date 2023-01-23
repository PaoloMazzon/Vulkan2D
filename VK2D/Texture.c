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
#include <malloc.h>

static void _vk2dTextureCreateDescriptor(VK2DTexture tex, VK2DRenderer renderer) {
	if (tex->img->set == NULL) {
		tex->img->set = vk2dDescConGetSamplerSet(renderer->descConSamplers, tex);
	}
}

VK2DTexture vk2dTextureLoadFromImage(VK2DImage image) {
	VK2DTexture out = calloc(1, sizeof(struct VK2DTexture));
	VK2DRenderer renderer = vk2dRendererGetPointer();

	if (vk2dPointerCheck(out)) {
		out->img = image;
		_vk2dTextureCreateDescriptor(out, renderer);
	} else {
		free(out);
		out = NULL;
	}

	return out;
}

VK2DTexture vk2dTextureLoad(const char *filename) {
	VK2DImage image;
	VK2DTexture out = NULL;
	image = vk2dImageLoad(vk2dRendererGetDevice(), filename);

	if (vk2dPointerCheck(image)) {
		out = vk2dTextureLoadFromImage(image);
		if (vk2dPointerCheck(out)) {
			_vk2dTextureCreateDescriptor(out, vk2dRendererGetPointer());
			out->imgHandled = true;
		} else {
			vk2dImageFree(image);
		}
	}

	return out;
}

void _vk2dCameraUpdateUBO(VK2DUniformBufferObject *ubo, VK2DCameraSpec *camera);
void _vk2dImageTransitionImageLayout(VK2DLogicalDevice dev, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
void _vk2dRendererAddTarget(VK2DTexture tex);
void _vk2dRendererRemoveTarget(VK2DTexture tex);
VK2DTexture vk2dTextureCreate(float w, float h) {
	VK2DTexture out = malloc(sizeof(struct VK2DTexture));
	VK2DRenderer renderer = vk2dRendererGetPointer();
	VK2DLogicalDevice dev = vk2dRendererGetDevice();

	// For the UBO
	VK2DCameraSpec cam = {
			ct_Default,
			0,
			0,
			w,
			h,
			1
	};
	VK2DUniformBufferObject ubo = {};
	_vk2dCameraUpdateUBO(&ubo, &cam);

	if (vk2dPointerCheck(out)) {
		out->img = vk2dImageCreate(dev, w, h, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1);
		out->sampledImg = vk2dImageCreate(dev, w, h, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, (VkSampleCountFlagBits)renderer->config.msaa);
		_vk2dImageTransitionImageLayout(dev, out->img->img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		_vk2dImageTransitionImageLayout(dev, out->sampledImg->img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// Set up FBO
		const int attachCount = renderer->config.msaa > 1 ? 2 : 1;
		VkImageView attachments[attachCount];
		if (renderer->config.msaa > 1) {
			attachments[0] = out->sampledImg->view;
			attachments[1] = out->img->view;
		} else {
			attachments[0] = out->img->view;
		}

		VkFramebufferCreateInfo framebufferCreateInfo = vk2dInitFramebufferCreateInfo(renderer->externalTargetRenderPass, w, h, attachments, attachCount);
		vk2dErrorCheck(vkCreateFramebuffer(dev->dev, &framebufferCreateInfo, VK_NULL_HANDLE, &out->fbo));

		// And the UBO
		out->ubo = vk2dBufferLoad(dev, sizeof(VK2DUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &ubo);
		out->uboSet = vk2dDescConGetBufferSet(renderer->descConVP, out->ubo);

		_vk2dRendererAddTarget(out);
		_vk2dTextureCreateDescriptor(out, renderer);
	} else {
		free(out);
		out = NULL;
	}

	return out;
}

void vk2dTextureFree(VK2DTexture tex) {
	if (tex != NULL) {
		if (tex->fbo != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(tex->img->dev->dev, tex->fbo, VK_NULL_HANDLE);
			vk2dImageFree(tex->img);
			vk2dBufferFree(tex->ubo);
			vk2dImageFree(tex->sampledImg);
			_vk2dRendererRemoveTarget(tex);
		} else if (tex->imgHandled) {
			vk2dImageFree(tex->img);
		}
		free(tex);
	}
}