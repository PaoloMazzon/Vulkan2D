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
#include <malloc.h>

// Will be modified to fit the texture then uploaded to a polygon
VK2DVertexTexture baseTex[] = {
		{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}
};
VK2DVertexTexture immutableFull[] = {
		{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}
};
const uint32_t baseTexVertexCount = 6;

VK2DTexture vk2dTextureLoad(VK2DImage image, float xInImage, float yInImage, float wInImage, float hInImage) {
	// In order to display portions of an image, we normalize UV coordinates and stick it in a vertex buffer
	float x1 = xInImage / image->width;
	float y1 = yInImage / image->height;
	float x2 = (xInImage + wInImage) / image->width;
	float y2 = (yInImage + hInImage) / image->height;

	// Image loads flipped so this is a really cheaty way of fixing
	float hold = x2;
	x2 = x1;
	x1 = hold;

	// Order of the 6 vertices are as follows:
	//     UR, UL, UR, UR, BR, BL
	// Where U is up or top, b is bottom, l is left, and r is right
	baseTex[0].tex[0] = x2;
	baseTex[0].tex[1] = y1;
	baseTex[1].tex[0] = x1;
	baseTex[1].tex[1] = y1;
	baseTex[2].tex[0] = x1;
	baseTex[2].tex[1] = y2;
	baseTex[3].tex[0] = x1;
	baseTex[3].tex[1] = y2;
	baseTex[4].tex[0] = x2;
	baseTex[4].tex[1] = y2;
	baseTex[5].tex[0] = x2;
	baseTex[5].tex[1] = y1;
	baseTex[1].pos[0] = wInImage;
	baseTex[2].pos[0] = wInImage;
	baseTex[2].pos[1] = hInImage;
	baseTex[3].pos[0] = wInImage;
	baseTex[3].pos[1] = hInImage;
	baseTex[4].pos[1] = hInImage;

	VK2DTexture out = calloc(1, sizeof(struct VK2DTexture));
	VK2DPolygon poly = vk2dPolygonTextureCreate(image->dev, baseTex, baseTexVertexCount);
	VK2DRenderer renderer = vk2dRendererGetPointer();

	if (vk2dPointerCheck(out) && vk2dPointerCheck(poly)) {
		out->imgSampler = &renderer->textureSampler;
		out->bounds = poly;
		out->img = image;
	} else {
		vk2dPolygonFree(poly);
		free(out);
		out = NULL;
	}

	return out;
}

void _vk2dCameraUpdateUBO(VK2DUniformBufferObject *ubo, VK2DCamera *camera);
VK2DTexture vk2dTextureCreate(VK2DLogicalDevice dev, float w, float h) {
	VK2DTexture out = malloc(sizeof(struct VK2DTexture));
	VK2DPolygon poly = vk2dPolygonTextureCreate(dev, (void*)immutableFull, baseTexVertexCount);
	VK2DRenderer renderer = vk2dRendererGetPointer();
	immutableFull[1].pos[0] = w;
	immutableFull[2].pos[0] = w;
	immutableFull[2].pos[1] = h;
	immutableFull[3].pos[0] = w;
	immutableFull[3].pos[1] = h;
	immutableFull[4].pos[1] = h;

	// For the UBO
	VK2DCamera cam = {
			0,
			0,
			w,
			h,
			1,
			0
	};
	VK2DUniformBufferObject ubo;
	_vk2dCameraUpdateUBO(&ubo, &cam);

	if (vk2dPointerCheck(out) && vk2dPointerCheck(poly)) {
		out->imgSampler = &renderer->textureSampler;
		out->bounds = poly;

		out->img = vk2dImageCreate(dev, w, h, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, (VkSampleCountFlagBits)renderer->config.msaa);

		// Set up FBO
		const int attachCount = renderer->config.msaa > 1 ? 3 : 2;
		VkImageView attachments[attachCount];
		if (renderer->config.msaa > 1) {
			attachments[0] = renderer->dsi->view;
			attachments[1] = renderer->msaaImage->view;
			attachments[2] = out->img->view;
		} else {
			attachments[0] = renderer->dsi->view;
			attachments[1] = out->img->view;
		}

		VkFramebufferCreateInfo framebufferCreateInfo = vk2dInitFramebufferCreateInfo(renderer->externalTargetRenderPass, w, h, attachments, attachCount);
		vk2dErrorCheck(vkCreateFramebuffer(dev->dev, &framebufferCreateInfo, VK_NULL_HANDLE, &out->fbo));

		// And the UBO
		out->ubo = vk2dBufferLoad(dev, sizeof(VK2DUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &ubo);
	} else {
		vk2dPolygonFree(poly);
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
		}
		vk2dPolygonFree(tex->bounds);
		free(tex);
	}
}