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
/// \param vertexData Vertex data of the polygon (must be triangulated)
/// \param vertexCount Number of vertices
/// \return Returns a new polygon
/// \warning This function is for creating fancier effects with the inner vertex data. Use vk2dPolygonCreate for creating simple shapes
VK2DPolygon vk2dPolygonTextureCreateRaw(VK2DLogicalDevice dev, VK2DVertexTexture *vertexData, uint32_t vertexCount);

/// \brief Creates a polygon for the shapes pipeline (should be triangulated)
/// \param dev Device to allocate the memory on
/// \param vertexData Vertex data of the polygon (must be triangulated)
/// \param vertexCount Number of vertices
/// \return Returns a new polygon
/// \warning This function is for creating fancier effects with the inner vertex data. Use vk2dPolygonCreate for creating simple shapes
VK2DPolygon vk2dPolygonShapeCreateRaw(VK2DLogicalDevice dev, VK2DVertexColour *vertexData, uint32_t vertexCount);

/// \brief Creates a polygon with specified vertices for drawing (use vk2dRendererSetColourMod to change colours)
/// \param dev Device to create the polygon with
/// \param vertices List of x/y positions for the polygon's vertices
/// \param vertexCount Amount of vertices
/// \return Returns either the new polygon or NULL if it failed
/// \warning Convex polygons only, the triangulation method will likely fail for concave polygons
/// \warning Resultant polygon ***MAY NOT*** be used with vk2dTextureCreateFrom
/// \warning Must have at least 3 vertices
/// \warning While you may use polygons created from this function with vk2dDrawPolygonOutline, it may not look like you expect and for that reason you may want vk2dPolygonCreateOutline for polygons you want to draw outlined
VK2DPolygon vk2dPolygonCreate(VK2DLogicalDevice dev, vec2 *vertices, uint32_t vertexCount);

/// \brief Creates a polygon made to be rendered as an outline (does not triangulate input)
/// \param dev Device to create the polygon on
/// \param vertices List of x/y positions for the polygons vertices
/// \param vertexCount Number of vertices in the list
/// \return Returns either the new polygon or NULL if it failed
/// \warning Resultant polygon ***MAY NOT*** be used with vk2dTextureCreateFrom
/// \warning It is not possible to draw a polygon made with this function with vk2dRendererDrawPolygon unless its a triangle
/// \warning This function will crash on < 1 vertex
VK2DPolygon vk2dPolygonCreateOutline(VK2DLogicalDevice dev, vec2 *vertices, uint32_t vertexCount);

/// \brief Frees a polygon from memory
/// \param polygon Polygon to free
void vk2dPolygonFree(VK2DPolygon polygon);

#ifdef __cplusplus
}
#endif