/// \file Gui.h
/// \author cmburn
/// \brief GUI code for Vulkan2D

#pragma once

#include "VK2D/nuklear_defs.h"
#include "VK2D/Structs.h"

void vk2dGuiLoadFonts(const struct VK2DFont *fonts, size_t count);
void vk2dEnsureFontsLoaded();

