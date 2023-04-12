/// \file Polygon.c
/// \author Paolo Mazzon
#include "VK2D/Polygon.h"
#include "VK2D/Buffer.h"
#include "VK2D/Validation.h"
#include "VK2D/Renderer.h"
#include "VK2D/Opaque.h"
#include <malloc.h>

VK2DPolygon _vk2dPolygonCreate(VK2DLogicalDevice dev, void *data, uint32_t size, VK2DVertexType type) {
	VK2DPolygon poly = malloc(sizeof(struct VK2DPolygon));
	VK2DBuffer buf = vk2dBufferLoad(dev, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data);
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

VK2DPolygon vk2dPolygonShapeCreateRaw(VK2DVertexColour *vertexData, uint32_t vertexCount) {
	VK2DLogicalDevice dev = vk2dRendererGetDevice();
	VK2DPolygon poly = _vk2dPolygonCreate(dev, vertexData, sizeof(VK2DVertexColour) * vertexCount, VK2D_VERTEX_TYPE_SHAPE);
	if (poly != NULL)
		poly->vertexCount = vertexCount;
	return poly;
}

/*
   * Triangulation algorithm
   *  1. Allocate a final vertex list containing (vertexCount - 2) * 3 elements
   *  2. Starting from the third vertex, and for each vertex proceeding it
   *    a. Add the first vertex in the polygon to the final vertex list
   *    b. Add the current vertex in the polygon to the final list
   *    c. Add the vertex before the current one in the polygon to the final list
   *  3. Push the final vertex list off to vk2dPolygonShapeCreateRaw
*/
VK2DPolygon vk2dPolygonCreate(vec2 *vertices, uint32_t vertexCount) {
	uint32_t finalVertexCount = (vertexCount - 2) * 3;
	VK2DVertexColour *colourVertices = malloc(sizeof(VK2DVertexColour) * finalVertexCount);
	uint32_t i;
	uint32_t v = 0; // Current element in final vertex list
	VK2DVertexColour defVert = {{0, 0, 0}, {1, 1, 1, 1}};
	VK2DPolygon out = NULL;
	VK2DLogicalDevice dev = vk2dRendererGetDevice();

	if (vk2dPointerCheck(colourVertices)) {
		for (i = 2; i < vertexCount; i++) {
			defVert.pos[0] = vertices[0][0];
			defVert.pos[1] = vertices[0][1];
			colourVertices[v++] = defVert;
			defVert.pos[0] = vertices[i][0];
			defVert.pos[1] = vertices[i][1];
			colourVertices[v++] = defVert;
			defVert.pos[0] = vertices[i - 1][0];
			defVert.pos[1] = vertices[i - 1][1];
			colourVertices[v++] = defVert;
		}

		out = vk2dPolygonShapeCreateRaw(colourVertices, finalVertexCount);
		free(colourVertices);
	}

	return out;
}

VK2DPolygon vk2dPolygonCreateOutline(vec2 *vertices, uint32_t vertexCount) {
	uint32_t i;
	VK2DVertexColour defVert = {{0, 0, 0}, {1, 1, 1, 1}};
	VK2DPolygon out = NULL;
	VK2DVertexColour *colourVertices = malloc(sizeof(VK2DVertexColour) * (vertexCount + 1));
	VK2DLogicalDevice dev = vk2dRendererGetDevice();

	if (vk2dPointerCheck(colourVertices)) {
		for (i = 0; i < vertexCount; i++) {
			defVert.pos[0] = vertices[i][0];
			defVert.pos[1] = vertices[i][1];
			colourVertices[i] = defVert;
		}
		defVert.pos[0] = vertices[0][0];
		defVert.pos[1] = vertices[0][1];
		colourVertices[vertexCount] = defVert;
		out = vk2dPolygonShapeCreateRaw(colourVertices, vertexCount + 1);
	}

	return out;
}

void vk2dPolygonFree(VK2DPolygon polygon) {
	if (polygon != NULL) {
		vk2dBufferFree(polygon->vertices);
		free(polygon);
	}
}