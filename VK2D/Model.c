/// \file Model.c
/// \author Paolo Mazzon
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <tinyobjloader-c/tinyobj_loader_c.h>

#include "VK2D/Model.h"
#include "VK2D/Buffer.h"
#include "VK2D/Renderer.h"
#include "VK2D/Validation.h"
#include "VK2D/Util.h"

// Needed to use buffers for tinyobjloader
static const void *gTinyObjBuffer;
static int gTinyObjBufferSize;

// For tinyobjloader
static void _getFileData(void* ctx, const char* filename, const int is_mtl,
						  const char* obj_filename, char** data, size_t* len) {
	*data = (void*)gTinyObjBuffer;
	*len = gTinyObjBufferSize;
}

VK2DModel vk2dModelCreate(const VK2DVertex3D *vertices, uint32_t vertexCount, const uint16_t *indices, uint32_t indexCount, VK2DTexture tex) {
	VK2DModel model = malloc(sizeof(struct VK2DModel));
	VK2DBuffer buf = vk2dBufferLoad2(vk2dRendererGetDevice(), sizeof(VK2DVertex3D) * vertexCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, vertices, sizeof(uint16_t) * indexCount, indices);

	if (vk2dPointerCheck(buf) && vk2dPointerCheck(model)) {
		model->vertices = buf;
		model->tex = tex;
		model->vertexCount = vertexCount;
		model->type = vt_Model;
		model->vertexOffset = 0;
		model->indexOffset = sizeof(VK2DVertex3D) * vertexCount;
		model->indexCount = indexCount;
	}

	return model;
}

VK2DModel vk2dModelFrom(const void *objFile, uint32_t objFileSize, VK2DTexture texture) {
	gTinyObjBuffer = objFile;
	gTinyObjBufferSize = objFileSize;
	VK2DModel m = NULL;
	bool error = false;

	tinyobj_attrib_t attrib;
	tinyobj_shape_t* shapes = NULL;
	size_t num_shapes;
	tinyobj_material_t* materials = NULL;
	size_t num_materials;
	int status = tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials, &num_materials, "", _getFileData, NULL, TINYOBJ_FLAG_TRIANGULATE);

	if (status == TINYOBJ_SUCCESS) {
		// Create lists for the worst case, actual parsed indices/vertices will likely we less
		VK2DVertex3D *vertices = malloc(sizeof(VK2DVertex3D) * attrib.num_face_num_verts);
		uint16_t *indices = malloc(sizeof(uint16_t) * attrib.num_face_num_verts);
		int vertexCount = 0;
		int indexCount = 0;

		if (vk2dPointerCheck(vertices) && vk2dPointerCheck(indices)) {
			// TODO: Parse vertices and create index list
			m = vk2dModelCreate(vertices, vertexCount, indices, indexCount, texture);
		} else {
			error = true;
		}
		free(vertices);
		free(indices);
		tinyobj_attrib_free(&attrib);
		tinyobj_shapes_free(shapes, num_shapes);
		tinyobj_materials_free(materials, num_materials);
	} else {
		error = true;
	}

	if (error)
		vk2dLogMessage("Failed to load model.");

	return m;
}

VK2DModel vk2dModelLoad(const char *objFile, VK2DTexture texture) {
	VK2DModel m = NULL;
	uint32_t size;
	const void *data = _vk2dLoadFile(objFile, &size);
	m = vk2dModelFrom(data, size, texture);
	free((void*)data);
	return m;
}

void vk2dModelFree(VK2DModel model) {
	if (model != NULL) {
		vk2dBufferFree(model->vertices);
		free(model);
	}
}