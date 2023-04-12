/// \file Model.c
/// \author Paolo Mazzon
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "VK2D/tinyobj_loader_c.h"

#include "VK2D/Model.h"
#include "VK2D/Buffer.h"
#include "VK2D/Renderer.h"
#include "VK2D/Validation.h"
#include "VK2D/Opaque.h"
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
		model->type = VK2D_VERTEX_TYPE_MODEL;
		model->vertexOffset = 0;
		model->indexOffset = sizeof(VK2DVertex3D) * vertexCount;
		model->indexCount = indexCount;
	}

	return model;
}

static inline bool verticesAreEqual(VK2DVertex3D *v1, VK2DVertex3D *v2) {
	if (v1->uv[0] == v2->uv[0] && v1->uv[1] == v2->uv[1] && v1->pos[0] == v2->pos[0] &&
			v1->pos[1] == v2->pos[1] && v1->pos[2] == v2->pos[2])
		return true;
	return false;
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
		VK2DVertex3D *vertices = malloc(sizeof(VK2DVertex3D) * attrib.num_faces * 3);
		uint16_t *indices = malloc(sizeof(uint16_t) * attrib.num_faces * 3);
		int vertexCount = 0;
		int indexCount = 0;

		if (vk2dPointerCheck(vertices) && vk2dPointerCheck(indices)) {
			for (int faceIndex = 0; faceIndex < attrib.num_faces; faceIndex++) {
				VK2DVertex3D vertex = {};

				// Build the vertex
				int vertexIndex = attrib.faces[faceIndex].v_idx;
				int textureIndex = attrib.faces[faceIndex].vt_idx;
				vertex.pos[0] = attrib.vertices[(vertexIndex * 3) + 0];
				vertex.pos[1] = attrib.vertices[(vertexIndex * 3) + 1];
				vertex.pos[2] = attrib.vertices[(vertexIndex * 3) + 2];
				vertex.uv[0] = attrib.texcoords[(textureIndex * 2) + 0];
				vertex.uv[1] = 1 - attrib.texcoords[(textureIndex * 2) + 1];

				// Check if this vertex is the same as any of the previous 6 (kinda arbitrary)
				int found = -1;
				for (int i = 0; i < vertexIndex && found == -1; i++)
					if (verticesAreEqual(&vertex, &vertices[i]))
						found = i;

				if (found != -1) {
					// Vertex is already at vertices[found]
					indices[indexCount++] = found;
				} else {
					// Vertex was not found in the vertex list
					vertices[vertexCount++] = vertex;
					indices[indexCount++] = vertexCount - 1;
				}
			}

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