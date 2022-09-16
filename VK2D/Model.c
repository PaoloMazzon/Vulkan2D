/// \file Model.c
/// \author Paolo Mazzon
#include "VK2D/Model.h"
#include "VK2D/Buffer.h"
#include "VK2D/Renderer.h"
#include "VK2D/Validation.h"

VK2DModel vk2dModelCreate(VK2DVertex3D *vertices, uint32_t vertexCount, VK2DTexture tex) {
	VK2DModel model = malloc(sizeof(struct VK2DModel));
	VK2DBuffer buf = vk2dBufferLoad(vk2dRendererGetDevice(), sizeof(VK2DVertex3D) * vertexCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices);

	if (vk2dPointerCheck(buf) && vk2dPointerCheck(model)) {
		model->vertices = buf;
		model->tex = tex;
		model->vertexCount = vertexCount;
		model->type = vt_Model;
	}

	return model;
}

void vk2dModelFree(VK2DModel model) {
	if (model != NULL) {
		vk2dBufferFree(model->vertices);
		free(model);
	}
}