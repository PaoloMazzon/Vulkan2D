/// \file Gui.h
/// \author cmburn
/// \brief GUI code for Vulkan2D

#pragma once

#include "VK2D/nuklear_defs.h"
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Returns the internal nuklear context for nuklear calls
/// \return Returns the internal nuklear context for nuklear calls
struct nk_context *vk2dGuiContext();

/// \brief Call this at the beginning of your SDL event loop
void vk2dGuiStartInput();

/// \brief Call this on each SDL event
/// \param e The current SDL event
void vk2dGuiProcessEvent(SDL_Event *e);

/// \brief Call this at the end of your SDL event loop
void vk2dGuiEndInput();

#ifdef __cplusplus
}
#endif
