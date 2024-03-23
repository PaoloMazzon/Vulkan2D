/// \file Polygon.h
/// \author Paolo Mazzon
/// \brief Abstraction of VK2DBuffer for shapes
#pragma once
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Creates a polygon for the shapes pipeline (should be triangulated)
/// \param vertexData Vertex data of the polygon (must be triangulated)
/// \param vertexCount Number of vertices
/// \return Returns a new polygon
/// \warning This function is for creating fancier effects with the inner vertex data. Use vk2dPolygonCreate for creating simple shapes
/// \warning Polygon must be triangulated unless it will be drawn as an outline, vk2dPolygonCreate automatically triangulates but this does not.
VK2DPolygon vk2dPolygonShapeCreateRaw(VK2DVertexColour *vertexData, uint32_t vertexCount);

/// \brief Creates a polygon with specified vertices for drawing (use vk2dRendererSetColourMod to change colours)
/// \param vertices List of x/y positions for the polygon's vertices
/// \param vertexCount Amount of vertices
/// \return Returns either the new polygon or NULL if it failed
/// \warning Convex polygons only, the triangulation method will likely fail for concave polygons
/// \warning Must have at least 3 vertices
/// \warning While you may use polygons created from this function with vk2dDrawPolygonOutline, it may not look like you expect and for that reason you may want vk2dPolygonCreateOutline for polygons you want to draw outlined
VK2DPolygon vk2dPolygonCreate(vec2 *vertices, uint32_t vertexCount);

/// \brief Creates a polygon made to be rendered as an outline (does not triangulate input)
/// \param vertices List of x/y positions for the polygons vertices
/// \param vertexCount Number of vertices in the list
/// \return Returns either the new polygon or NULL if it failed
/// \warning It is not possible to draw a polygon made with this function with vk2dRendererDrawPolygon unless its a triangle
/// \warning This function will crash on < 1 vertex
VK2DPolygon vk2dPolygonCreateOutline(vec2 *vertices, uint32_t vertexCount);

/// \brief Frees a polygon from memory
/// \param polygon Polygon to free
void vk2dPolygonFree(VK2DPolygon polygon);

#ifdef __cplusplus
}
#endif