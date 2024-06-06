/// \file ShadowEnvironment.h
/// \author Paolo Mazzon
/// \brief Information needed to use hardware-accelerated shadows
#pragma once
#include "VK2D/Structs.h"
#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Creates a new shadow environment
/// \return An empty shadow environment.
VK2DShadowEnvironment vk2DShadowEnvironmentCreate();

/// \brief Frees a shadow environment
/// \param shadowEnvironment Shadow environment to free
void vk2DShadowEnvironmentFree(VK2DShadowEnvironment shadowEnvironment);

/// \brief Adds a new object to a shadow environment
/// \param shadowEnvironment Shadow environment
/// \returns Shadow object index, or VK2D_INVALID_SHADOW_OBJECT if this fails
VK2DShadowObject vk2dShadowEnvironmentAddObject(VK2DShadowEnvironment shadowEnvironment);

/// \brief Translates a shadow object
/// \param object Object to update
/// \param x X position of the object
/// \param y Y position of the object
void vk2dShadowEnvironmentObjectTranslate(VK2DShadowObject object, float x, float y);

/// \brief Updates a shadow object
/// \param object Object to update
/// \param x X position of the object
/// \param y Y position of the object
/// \param scaleX X scale of the object
/// \param scaleY Y scale of the object
/// \param rotation Rotation of the object
/// \param originX X origin of the object
/// \param originY Y origin of the object
void vk2dShadowEnvironmentObjectUpdate(VK2DShadowObject object, float x, float y, float scaleX, float scaleY, float rotation, float originX, float originY);

/// \brief Adds an edge to a shadow environment's current object, use this on wall edges
/// \param shadowEnvironment Shadow environment to add to
/// \param x1 X of the start of the edge
/// \param y1 Y of the start of the edge
/// \param x2 X of the end of the edge
/// \param y2 Y of the end of the edge
///
/// You may call this from another thread so long as you do not call vk2DShadowEnvironmentFlushVBO
/// at the same time (ie, set up all your edges in the other thread and
/// once you know that thread is done adding edges call vk2DShadowEnvironmentFlushVBO).
void vk2DShadowEnvironmentAddEdge(VK2DShadowEnvironment shadowEnvironment, float x1, float y1, float x2, float y2);

/// \brief Removes all edges from the shadow environment's cache in case you want to change the edges
/// \param shadowEnvironment Shadow environment to reset
/// \warning This invalidates all objects previously got from vk2dShadowEnvironmentAddObject
void vk2dShadowEnvironmentResetEdges(VK2DShadowEnvironment shadowEnvironment);

/// \brief Flushes the edges present in the environment to a VBO that can be drawn to screen
/// \param shadowEnvironment Shadow environment to flush
/// \warning This function has heavy overhead and should not be called every frame.
void vk2DShadowEnvironmentFlushVBO(VK2DShadowEnvironment shadowEnvironment);

#ifdef __cplusplus
}
#endif