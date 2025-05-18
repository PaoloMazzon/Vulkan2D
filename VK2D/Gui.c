/// \file Initializers.h
/// \author cmburn
/// \brief Logging callbacks

#include "VK2D/Gui.h"
#include "VK2D/Logger.h"
#include "VK2D/Opaque.h"
#include "VK2D/Renderer.h"
#include "VK2D/uthash.h"

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

static bool
render(VkSemaphore semaphore)
{
	// https://github.com/Immediate-Mode-UI/Nuklear/blob/master/demo/sdl_vulkan/main.c#L1904
	return true;
}

bool
vk2dGuiRender()
{
	VK2DRenderer vk2d = vk2dRendererGetPointer();
	const bool useMsaa = vk2d->config.msaa > VK2D_MSAA_1X;
	const enum nk_anti_aliasing msaa
	    = useMsaa ? NK_ANTI_ALIASING_ON : NK_ANTI_ALIASING_OFF;
	const uint32_t imageIndex = vk2d->scImageIndex;
	VkSemaphore semaphore = nk_sdl_render(vk2d->ld->queue, imageIndex,
	    vk2d->imageAvailableSemaphores[imageIndex], msaa);
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

static enum nk_panel_flags
windowFlags(SDL_Window *window)
{
	SDL_WindowFlags flags = SDL_GetWindowFlags(window);
}

struct nk_rect
windowShape(SDL_Window *window)
{
	int x, y, w, h;
	SDL_GetWindowPosition(window, &x, &y);
	SDL_GetWindowSize(window, &w, &h);
	return (struct nk_rect) {
		.x = (float)x,
		.y = (float)y,
		.w = (float)w,
		.h = (float)h,
	};
}

bool
vk2dGuiStart()
{
	VK2DRenderer vk2d = vk2dRendererGetPointer();
	SDL_Window *win = vk2d->window;
	const char *title = SDL_GetWindowTitle(win);
	return nk_begin(vk2d->gui->context, title, windowShape(win),
	    windowFlags(win));
}

void
vk2dGuiEnd()
{
	VK2DRenderer vk2d = vk2dRendererGetPointer();
	nk_end(vk2d->gui->context);
}

struct nk_context *
vk2dGuiContext()
{
	VK2DRenderer vk2d = vk2dRendererGetPointer();
	return vk2d->gui->context;
}
