/// \file Initializers.h
/// \author cmburn
/// \brief Logging callbacks

#include "VK2D/Gui.h"
#include "VK2D/Logger.h"
#include "VK2D/Opaque.h"
#include "VK2D/Renderer.h"
#include "VK2D/uthash.h"

void vk2dGuiStartInput() {
	VK2DRenderer vk2d = vk2dRendererGetPointer();
	if (!vk2d->options.enableNuklear) return;
	nk_input_begin(vk2d->gui->context);
}

void vk2dGuiEndInput() {
	VK2DRenderer vk2d = vk2dRendererGetPointer();
	if (!vk2d->options.enableNuklear) return;
	nk_input_end(vk2d->gui->context);
}

void vk2dGuiProcessEvent(SDL_Event *e) {
    VK2DRenderer vk2d = vk2dRendererGetPointer();
    if (!vk2d->options.enableNuklear) return;
	nk_sdl_handle_event(e);
}

struct nk_context * vk2dGuiContext() {
	VK2DRenderer vk2d = vk2dRendererGetPointer();
	return vk2d->gui->context;
}
