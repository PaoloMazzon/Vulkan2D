#define SDL_MAIN_HANDLED
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <SDL3/SDL_vulkan.h>
#include "../debug.c"
#include "VK2D/Gui.h"
#include "VK2D/Logger.h"
#include "VK2D/Structs.h"
#include "VK2D/VK2D.h"

/************************ Constants ************************/

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

int
main(int argc, const char *argv[])
{
	// Basic SDL setup
	SDL_Init(SDL_INIT_EVENTS);
	SDL_Window *window = SDL_CreateWindow("VK2D", WINDOW_WIDTH,
	    WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	SDL_Event e;
	bool quit = false;
	int keyboardSize;
	const bool *keyboard = SDL_GetKeyboardState(&keyboardSize);
	if (window == NULL) return -1;

	// Initialize vk2d
	VK2DRendererConfig config = { VK2D_MSAA_1X, VK2D_SCREEN_MODE_IMMEDIATE,
		VK2D_FILTER_TYPE_NEAREST };
	vec4 clear = { 0.0, 0.5, 1.0, 1.0 };
	VK2DStartupOptions options = {
		.quitOnError = true,
		.enableDebug = false,
		.stdoutLogging = true,
		.vramPageSize = sizeof(VK2DDrawInstance) * 2000010,
	};
	vk2dRendererInit(window, config, &options);
	debugInit(window);
	static const struct VK2DFont fonts[] = {
		{
			.filename = "assets/spleen.otf",
			.config = NULL,
			.name = "Spleen 32x64",
			.height = 12,
			.inMemory = false,
		},
	};
	vk2dGuiLoadFonts(fonts, 1);

	// Delta and fps
	while (!quit && !vk2dStatusFatal()) {
		// const double time
		//     = (double)(SDL_GetPerformanceCounter() - startTime)
		//     / (double)SDL_GetPerformanceFrequency();


		vk2dGuiStartInput();
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) { quit = true; }
			nk_sdl_handle_event(&e);
		}
		vk2dGuiEndInput();
		SDL_PumpEvents();
		int windowWidth, windowHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);

		vk2dRendererStartFrame(clear);

		// vk2dRendererFlushSpriteBatch();

		if (vk2dGuiStart()) {
			struct nk_context *ctx = vk2dGuiContext();
			nk_layout_row_static(ctx, 30, 80, 1);
			if (nk_button_label(ctx, "button"))
				fprintf(stdout, "button pressed\n");
		}
		vk2dGuiEnd();

		debugRenderOverlay();
		if (!vk2dGuiRender()) {
			vk2dLogFatal("Failed to render GUI");
		}
		vk2dRendererEndFrame();
	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	debugCleanup();
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}
