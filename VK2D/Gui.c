/// \file Initializers.h
/// \author cmburn
/// \brief Logging callbacks

#include "VK2D/Gui.h"
#include "VK2D/Logger.h"
#include "VK2D/Opaque.h"
#include "VK2D/Renderer.h"
#include "VK2D/uthash.h"

struct nk_font *
addFont(struct nk_font_atlas *atlas, const struct VK2DGuiFont *font)
{
	if (font->inMemory) {
		vk2dLogInfo("Loading font \"%s\" from memory", font->name);
		return nk_font_atlas_add_from_memory(atlas, font->data,
		    font->size, font->height, font->config);
	}
	vk2dLogInfo("Loading font \"%s\" from file \"%s\"", font->name,
	    font->filename);
	return nk_font_atlas_add_from_file(atlas, font->filename, font->height,
	    font->config);
}

void
vk2dGuiLoadFonts(const VK2DGuiFont *fonts, size_t count)
{
	VK2DRenderer vk2d = vk2dRendererGetPointer();
	VK2DGui gui = vk2d->gui;
	struct nk_font_atlas *atlas;
	nk_sdl_font_stash_begin(&atlas);
	if (fonts == NULL) {
		vk2dLogInfo("No fonts specified, proceeding with default");
		goto done;
	}
	if (gui->fonts != NULL) {
		vk2dLogInfo("Fonts already loading, clearing old fonts");
		free(gui->fonts);
		gui->fonts = NULL;
	}
	for (size_t i = 0; i < count; i++) {
		struct nk_font *font = addFont(atlas, &fonts[i]);
		assert(font != NULL);

		struct VK2DFontHandle *handle = malloc(sizeof(*handle));
		*handle = (struct VK2DFontHandle) {
			.hh = {},
			.name = strdup(fonts[i].name),
			.font = font,
		};
		HASH_ADD_KEYPTR(hh, gui->fonts, handle->name,
		    strlen(handle->name), handle);
	}
	gui->fontsCount = count;
	gui->fontsLoaded = true;
done:
	nk_sdl_font_stash_end(vk2d->ld->queue);
}

void
vk2dGuiStartInput()
{
	VK2DRenderer vk2d = vk2dRendererGetPointer();
	nk_input_begin(vk2d->gui->context);
}

void
vk2dGuiEndInput()
{
	VK2DRenderer vk2d = vk2dRendererGetPointer();
	nk_input_end(vk2d->gui->context);
}

void vk2dGuiProcessEvent(SDL_Event *e) {
	nk_sdl_handle_event(e);
}

struct nk_context *
vk2dGuiContext()
{
	VK2DRenderer vk2d = vk2dRendererGetPointer();
	return vk2d->gui->context;
}
