/// \file Texture.c
/// \author Paolo Mazzon
#include "VK2D/Texture.h"
#include "VK2D/Image.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/Polygon.h"
#include <malloc.h>

VK2DTexture vk2dTextureLoad(VK2DImage image, float xInImage, float yInImage, float wInImage, float hInImage) {
	return NULL; // TODO: This
}

VK2DTexture vk2dTextureCreate(VK2DLogicalDevice dev, float w, float h) {
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