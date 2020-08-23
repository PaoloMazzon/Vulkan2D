/// \file Polygon.c
/// \author Paolo Mazzon
#include "VK2D/Polygon.h"
#include "VK2D/Buffer.h"
#include "VK2D/Validation.h"
#include <malloc.h>

VK2DPolygon _vk2dPolygonCreate(VK2DLogicalDevice dev, void *data, uint32_t size, VK2DVertexType type) {
	VK2DPolygon poly = malloc(sizeof(struct VK2DPolygon));
	VK2DBuffer buf = vk2dBufferLoad(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, dev, data);
	if (vk2dPointerCheck(buf) && vk2dPointerCheck(poly)) {
		poly->vertices = buf;
		poly->type = type;
	} else {
		free(poly);
		vk2dBufferFree(buf);
		poly = NULL;
	}
	return poly;
}

VK2DPolygon vk2dPolygonTextureCreate(VK2DLogicalDevice dev, VK2DVertexTexture *vertexData, uint32_t vertexCount) {
	return _vk2dPolygonCreate(dev, vertexData, sizeof(VK2DVertexTexture) * vertexCount, vt_Texture);
}

VK2DPolygon vk2dPolygonShapeCreate(VK2DLogicalDevice dev, VK2DVertexColour *vertexData, uint32_t vertexCount) {
	return _vk2dPolygonCreate(dev, vertexData, sizeof(VK2DVertexColour) * vertexCount, vt_Shape);
}

void vk2dPolygonFree(VK2DPolygon polygon) {
	if (polygon != NULL) {
		vk2dBufferFree(polygon->vertices);
		free(polygon);
	}
}