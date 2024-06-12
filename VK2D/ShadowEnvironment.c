/// \file ShadowEnvironment.c
/// \author Paolo Mazzon
#include "VK2D/ShadowEnvironment.h"
#include "VK2D/Validation.h"
#include "VK2D/Buffer.h"
#include "VK2D/Opaque.h"
#include "VK2D/Constants.h"
#include "VK2D/Renderer.h"

// From Math.h
void identityMatrix(float m[]);
void scaleMatrix(float m[], float v[]);
void translateMatrix(float m[], float v[]);
void rotateMatrix(float m[], float w[], float r);

VK2DShadowEnvironment vk2DShadowEnvironmentCreate() {
    VK2DShadowEnvironment se = malloc(sizeof(struct VK2DShadowEnvironment_t));

    if (se) {
        se->vboVertexSize = 0;
        se->vbo = NULL;
        se->vertices = NULL;
        se->verticesSize = 0;
        se->verticesCount = 0;
        se->objectCount = 0;
        se->objectInfos = NULL;
        vk2dShadowEnvironmentAddObject(se);
    } else {
        vk2dLogMessage("Failed to create shadow environment.");
    }

    return se;
}

void vk2DShadowEnvironmentFree(VK2DShadowEnvironment shadowEnvironment) {
    if (shadowEnvironment != NULL) {
        free(shadowEnvironment->vertices);
        vk2dBufferFree(shadowEnvironment->vbo);
        free(shadowEnvironment->objectInfos);
        free(shadowEnvironment);
    }
}

VK2DShadowObject vk2dShadowEnvironmentAddObject(VK2DShadowEnvironment shadowEnvironment) {
    VK2DShadowObject so = VK2D_INVALID_SHADOW_OBJECT;

    // In case the user doesn't use the default one
    if (shadowEnvironment->verticesCount == 0 && shadowEnvironment->objectCount > 0 && shadowEnvironment->objectInfos[0].vertexCount == 0)
        return 0;

    // Reallocate object array
    void *newMem = realloc(shadowEnvironment->objectInfos, sizeof(VK2DShadowObjectInfo) * (shadowEnvironment->objectCount + 1));

    if (newMem != NULL) {
        so = shadowEnvironment->objectCount;
        shadowEnvironment->objectCount++;
        shadowEnvironment->objectInfos = newMem;
        VK2DShadowObjectInfo *soi = &shadowEnvironment->objectInfos[shadowEnvironment->objectCount - 1];
        soi->vertexCount = 0;
        soi->startingVertex = shadowEnvironment->verticesCount;
        soi->enabled = true;
        memset(soi->model, 0, sizeof(mat4));
        identityMatrix(soi->model);
    } else {
        vk2dLogMessage("Failed to create shadow object.");
    }

    return so;
}

void vk2dShadowEnvironmentObjectSetPos(VK2DShadowEnvironment shadowEnvironment, VK2DShadowObject object, float x, float y) {
    vec3 origin = {x, y, 0};
    memset(shadowEnvironment->objectInfos[object].model, 0, sizeof(mat4));
    identityMatrix(shadowEnvironment->objectInfos[object].model);
    translateMatrix(shadowEnvironment->objectInfos[object].model, origin);
}

void vk2dShadowEnvironmentObjectUpdate(VK2DShadowEnvironment shadowEnvironment, VK2DShadowObject object, float x, float y, float scaleX, float scaleY, float rotation, float originX, float originY) {
    memset(shadowEnvironment->objectInfos[object].model, 0, sizeof(mat4));
    identityMatrix(shadowEnvironment->objectInfos[object].model);
    // Only do rotation matrices if a rotation is specified for optimization purposes
    if (rotation != 0) {
        vec3 axis = {0, 0, 1};
        vec3 origin = {-originX + x, originY + y, 0};
        vec3 originTranslation = {originX, -originY, 0};
        translateMatrix(shadowEnvironment->objectInfos[object].model, origin);
        rotateMatrix(shadowEnvironment->objectInfos[object].model, axis, rotation);
        translateMatrix(shadowEnvironment->objectInfos[object].model, originTranslation);
    } else {
        vec3 origin = {x, y, 0};
        translateMatrix(shadowEnvironment->objectInfos[object].model, origin);
    }
    // Only scale matrix if specified for optimization purposes
    if (scaleX != 1 || scaleY != 1) {
        vec3 scale = {scaleX, scaleY, 1};
        scaleMatrix(shadowEnvironment->objectInfos[object].model, scale);
    }
}

void vk2dShadowEnvironmentObjectSetStatus(VK2DShadowEnvironment shadowEnvironment, VK2DShadowObject object, bool enabled) {
    shadowEnvironment->objectInfos[object].enabled = enabled;
}

bool vk2dShadowEnvironmentObjectGetStatus(VK2DShadowEnvironment shadowEnvironment, VK2DShadowObject object) {
    return shadowEnvironment->objectInfos[object].enabled;
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
                {x2, y2, 0},
                {x2, y2, 0}, // Projected - 1
                {x1, y1, 0},
                {x1, y1, 0}, // Projected - 3
                {x1, y1, 0}, // Projected - 4
                {x2, y2, 0},
        };

        memcpy(&shadowEnvironment->vertices[shadowEnvironment->verticesCount], vert, sizeof(vec3) * 6);
        shadowEnvironment->verticesCount += 6;
        shadowEnvironment->objectInfos[shadowEnvironment->objectCount - 1].vertexCount += 6;
    }
}

void vk2dShadowEnvironmentResetEdges(VK2DShadowEnvironment shadowEnvironment) {
    shadowEnvironment->verticesCount = 0;
    free(shadowEnvironment->objectInfos);
    shadowEnvironment->objectInfos = NULL;
    shadowEnvironment->objectCount = 0;
    vk2dShadowEnvironmentAddObject(shadowEnvironment);
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