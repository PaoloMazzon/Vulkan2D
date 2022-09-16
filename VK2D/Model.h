/// \file Model.h
/// \author Paolo Mazzon
/// \brief Declares things related to loading and managing models
#pragma once
#include "VK2D/Structs.h"

/// \brief A 3D model
struct VK2DModel {
	VK2DBuffer vertices;  ///< Internal memory for the vertices
	VK2DVertexType type;  ///< What kind of vertices this stores
	uint32_t vertexCount; ///< Number of vertices
	VK2DTexture tex;      ///< Texture for this model
};

/// \brief Creates a model from a set of vertices
VK2DModel vk2dModelCreate(VK2DVertex3D *vertices, uint32_t vertexCount, VK2DTexture tex);

/// \brief The texture stored in the model is not destroyed
void vk2dModelDestroy(VK2DModel model);