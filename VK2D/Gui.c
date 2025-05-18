/// \file Initializers.h
/// \author cmburn
/// \brief Logging callbacks

#include "Gui.h"
#include "Logger.h"
#include "Opaque.h"
#include "Renderer.h"
#include "uthash.h"

struct nk_font *
addFont(struct nk_font_atlas *atlas, const struct VK2DFont *font)
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
vk2dGuiLoadFonts(const struct VK2DFont *fonts, size_t count)
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
done:
	gui->fontsLoaded = true;
	nk_sdl_font_stash_end(vk2d->ld->queue);
}
