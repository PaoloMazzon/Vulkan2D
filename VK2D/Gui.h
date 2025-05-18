/// \file Gui.h
/// \author cmburn
/// \brief GUI code for Vulkan2D

#pragma once

#include "VK2D/nuklear_defs.h"
#include "VK2D/Structs.h"

bool vk2dGuiRender();
struct nk_context *vk2dGuiContext();
void vk2dGuiLoadFonts(const struct VK2DFont *fonts, size_t count);
void vk2dGuiStartInput();
void vk2dGuiEndInput();
bool vk2dGuiStart();
void vk2dGuiEnd();
