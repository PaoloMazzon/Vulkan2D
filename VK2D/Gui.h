/// \file Gui.h
/// \author cmburn
/// \brief GUI code for Vulkan2D

#pragma once

#include "VK2D/nuklear_defs.h"
#include "VK2D/Structs.h"

/// \brief Returns the internal nuklear context for nuklear calls
/// \return Returns the internal nuklear context for nuklear calls
struct nk_context *vk2dGuiContext();

/// \brief Loads a number of fonts
/// \param fonts List of fonts to load
/// \param count Number of fonts in the list
void vk2dGuiLoadFonts(const VK2DGuiFont *fonts, size_t count);

/// \brief Call this at the beginning of your SDL event loop
void vk2dGuiStartInput();

/// \brief Call this on each SDL event
/// \param e The current SDL event
void vk2dGuiProcessEvent(SDL_Event *e);

/// \brief Call this at the end of your SDL event loop
void vk2dGuiEndInput();
