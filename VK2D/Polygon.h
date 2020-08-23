/// \file Polygon.h
/// \author Paolo Mazzon
/// \brief Abstraction of VK2DBuffer for shapes
#pragma once
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Makes shapes easier to deal with
struct VK2DPolygon {
	VK2DBuffer vertices;  ///< Internal memory for the vertices
	VK2DVertexType type;  ///< What kind of vertices this stores
	uint32_t vertexCount; ///< Number of vertices
};

/// \brief Creates a polygon for the texture pipeline (should be triangulated)
/// \param dev Device to allocate the memory on
/// \param vertexData Vertex data of the triangle
/// \param vertexCount Number of vertices
/// \return Returns a new polygon
VK2DPolygon vk2dPolygonTextureCreate(VK2DLogicalDevice dev, VK2DVertexTexture *vertexData, uint32_t vertexCount);

/// \brief Creates a polygon for the shapes pipeline (should be triangulated)
/// \param dev Device to allocate the memory on
/// \param vertexData Vertex data of the triangle
/// \param vertexCount Number of vertices
/// \return Returns a new polygon
VK2DPolygon vk2dPolygonShapeCreate(VK2DLogicalDevice dev, VK2DVertexColour *vertexData, uint32_t vertexCount);

/// \brief Frees a polygon from memory
/// \param polygon Polygon to free
void vk2dPolygonFree(VK2DPolygon polygon);

#ifdef __cplusplus
}
#endif