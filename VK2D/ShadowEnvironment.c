/// \file ShadowEnvironment.c
/// \author Paolo Mazzon
#include "VK2D/ShadowEnvironment.h"
#include "VK2D/Validation.h"
#include "VK2D/Buffer.h"
#include "VK2D/Opaque.h"
#include "VK2D/Constants.h"
#include "VK2D/Renderer.h"

VK2DShadowEnvironment vk2DShadowEnvironmentCreate() {
    VK2DShadowEnvironment se = malloc(sizeof(struct VK2DShadowEnvironment_t));

    if (se) {
        se->vboVertexSize = 0;
        se->vbo = NULL;
        se->vertices = NULL;
        se->verticesSize = 0;
        se->verticesCount = 0;
    } else {
        vk2dLogMessage("Failed to create shadow environment.");
    }

    return se;
}

void vk2DShadowEnvironmentFree(VK2DShadowEnvironment shadowEnvironment) {
    if (shadowEnvironment != NULL) {
        free(shadowEnvironment->vertices);
        vk2dBufferFree(shadowEnvironment->vbo);
        free(shadowEnvironment);
    }
}

void vk2DShadowEnvironmentAddEdge(VK2DShadowEnvironment shadowEnvironment, float x1, float y1, float x2, float y2) {
    bool failedToExtendList = false;

    // Extend list
    if (shadowEnvironment->verticesCount == shadowEnvironment->verticesSize) {
        vec3* newList = realloc(shadowEnvironment->vertices, (shadowEnvironment->verticesSize + (VK2D_DEFAULT_ARRAY_EXTENSION * 6)) * sizeof(vec3));

        if (newList) {
            shadowEnvironment->vertices = newList;
            shadowEnvironment->verticesSize += VK2D_DEFAULT_ARRAY_EXTENSION * 6;
        } else {
            failedToExtendList = true;
            vk2dLogMessage("Failed to extend shadow list");
        }
    }

    // Add the new 6 vertices for this edge
    if (!failedToExtendList) {
        vec3 vert[6] = {
                {x1, y1, 0},
                {x2, y1, 0}, // Projected - 1
                {x1, y2, 0},
                {x1, y2, 0}, // Projected - 3
                {x2, y1, 0}, // Projected - 4
                {x2, y2, 0},
        };

        memcpy(&shadowEnvironment->vertices[shadowEnvironment->verticesCount], vert, sizeof(vec3) * 6);
        shadowEnvironment->verticesCount += 6;
    }
}

void vk2dShadowEnvironmentResetEdges(VK2DShadowEnvironment shadowEnvironment) {
    shadowEnvironment->verticesCount = 0;
}

void vk2DShadowEnvironmentFlushVBO(VK2DShadowEnvironment shadowEnvironment) {
    vk2dRendererWait();
    vk2dBufferFree(shadowEnvironment->vbo);
    shadowEnvironment->vboVertexSize = shadowEnvironment->verticesCount;
    shadowEnvironment->vbo = vk2dBufferLoad(
            vk2dRendererGetDevice(),
            sizeof(vec3) * shadowEnvironment->verticesCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            shadowEnvironment->vertices, true
    );
}