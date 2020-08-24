/// \file Texture.c
/// \author Paolo Mazzon
#include "VK2D/Texture.h"
#include "VK2D/Image.h"
#include "VK2D/Initializers.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/Validation.h"
#include "VK2D/Polygon.h"
#include <malloc.h>

// Will be modified to fit the texture then uploaded to a polygon
VK2DVertexTexture baseTex[] = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}
};
const VK2DVertexTexture immutableFull[] = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}
};
const uint32_t baseTexVertexCount = 6;

VK2DTexture vk2dTextureLoad(VK2DImage image, float xInImage, float yInImage, float wInImage, float hInImage, VkBool32 filter, float anisotropy) {
	VK2DTexture out = malloc(sizeof(struct VK2DTexture));
	VK2DPolygon poly = vk2dPolygonTextureCreate(image->dev, baseTex, baseTexVertexCount);

	if (vk2dPointerCheck(out) && vk2dPointerCheck(poly)) {
		VkSamplerCreateInfo samplerCreateInfo = vk2dInitSamplerCreateInfo(filter, anisotropy, 1);
		vk2dErrorCheck(vkCreateSampler(image->dev->dev, &samplerCreateInfo, VK_NULL_HANDLE, &out->imgSampler));

		out->bounds = poly;
		out->img = image;
		out->fbo = NULL;

	} else {
		vk2dPolygonFree(poly);
		free(out);
		out = NULL;
	}

	return out;
}

VK2DTexture vk2dTextureCreate(VK2DLogicalDevice dev, float w, float h, VkBool32 filter, float anisotropy) {
	return NULL; // TODO: This
}

void vk2dTextureFree(VK2DTexture tex) {
	if (tex != NULL) {
		if (tex->fbo != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(tex->img->dev->dev, tex->fbo, VK_NULL_HANDLE);
			vk2dImageFree(tex->img);
		}
		vkDestroySampler(tex->img->dev->dev, tex->imgSampler, VK_NULL_HANDLE);
		vk2dPolygonFree(tex->bounds);
		free(tex);
	}
}