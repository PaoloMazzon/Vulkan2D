#define SDL_MAIN_HANDLED
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <SDL3/SDL_vulkan.h>
#include "../debug.c"
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
	if (window == NULL) return -1;

	// Initialize vk2d
	vec4 clear = { 0.0, 0.5, 1.0, 1.0 };
	VK2DRendererConfig config = {
	    .msaa = VK2D_MSAA_1X,
	    .screenMode = VK2D_SCREEN_MODE_TRIPLE_BUFFER,
		.filterMode = VK2D_FILTER_TYPE_NEAREST
	};
	VK2DStartupOptions options = {
		.quitOnError = true,
		.enableDebug = true,
		.stdoutLogging = true,
	    .enableNuklear = true,
	};
	vk2dRendererInit(window, config, &options);

	debugInit(window);
	static const struct VK2DGuiFont fonts[] = {
		{
			.filename = "assets/spleen.otf",
			.config = NULL,
			.name = "Spleen 32x64",
			.height = 12,
			.inMemory = false,
		},
	};
	//vk2dGuiLoadFonts(fonts, 1);
	float value = 0;
	int op = 0;

	// Delta and fps
	while (!quit && !vk2dStatusFatal()) {
		vk2dGuiStartInput();
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) { quit = true; }
			vk2dGuiProcessEvent(&e);
		}
		vk2dGuiEndInput();

		vk2dRendererStartFrame(clear);

        if (nk_begin(vk2dGuiContext(), "Show", nk_rect(50, 50, 220, 220),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE)) {
            // fixed widget pixel width
            nk_layout_row_static(vk2dGuiContext(), 30, 80, 1);
            if (nk_button_label(vk2dGuiContext(), "button")) {
                // event handling
            }

            // fixed widget window ratio width
            nk_layout_row_dynamic(vk2dGuiContext(), 30, 2);
            if (nk_option_label(vk2dGuiContext(), "easy", op == 0)) op = 0;
            if (nk_option_label(vk2dGuiContext(), "hard", op == 1)) op = 1;

            // custom widget pixel width
            nk_layout_row_begin(vk2dGuiContext(), NK_STATIC, 30, 2);
            {
                nk_layout_row_push(vk2dGuiContext(), 50);
                nk_label(vk2dGuiContext(), "Volume:", NK_TEXT_LEFT);
                nk_layout_row_push(vk2dGuiContext(), 110);
                nk_slider_float(vk2dGuiContext(), 0, &value, 1.0f, 0.1f);
            }
            nk_layout_row_end(vk2dGuiContext());
        }
        nk_end(vk2dGuiContext());

        if (nk_begin(vk2dGuiContext(), "Thing", nk_rect(220 + 60, 50, 220, 220),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE)) {
            // fixed widget pixel width
            nk_layout_row_static(vk2dGuiContext(), 30, 80, 1);
            if (nk_button_label(vk2dGuiContext(), "button")) {
                // event handling
            }

            // fixed widget window ratio width
            nk_layout_row_dynamic(vk2dGuiContext(), 30, 2);
            if (nk_option_label(vk2dGuiContext(), "easy", op == 0)) op = 0;
            if (nk_option_label(vk2dGuiContext(), "hard", op == 1)) op = 1;

            // custom widget pixel width
            nk_layout_row_begin(vk2dGuiContext(), NK_STATIC, 30, 2);
            {
                nk_layout_row_push(vk2dGuiContext(), 50);
                nk_label(vk2dGuiContext(), "Volume:", NK_TEXT_LEFT);
                nk_layout_row_push(vk2dGuiContext(), 110);
                nk_slider_float(vk2dGuiContext(), 0, &value, 1.0f, 0.1f);
            }
            nk_layout_row_end(vk2dGuiContext());
        }
        nk_end(vk2dGuiContext());

        if (nk_begin(vk2dGuiContext(), "Thing 2", nk_rect(50, 60 + 220, 220, 220),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE)) {
            // fixed widget pixel width
            nk_layout_row_static(vk2dGuiContext(), 30, 80, 1);
            if (nk_button_label(vk2dGuiContext(), "button")) {
                // event handling
            }

            // fixed widget window ratio width
            nk_layout_row_dynamic(vk2dGuiContext(), 30, 2);
            if (nk_option_label(vk2dGuiContext(), "easy", op == 0)) op = 0;
            if (nk_option_label(vk2dGuiContext(), "hard", op == 1)) op = 1;

            // custom widget pixel width
            nk_layout_row_begin(vk2dGuiContext(), NK_STATIC, 30, 2);
            {
                nk_layout_row_push(vk2dGuiContext(), 50);
                nk_label(vk2dGuiContext(), "Volume:", NK_TEXT_LEFT);
                nk_layout_row_push(vk2dGuiContext(), 110);
                nk_slider_float(vk2dGuiContext(), 0, &value, 1.0f, 0.1f);
            }
            nk_layout_row_end(vk2dGuiContext());
        }
        nk_end(vk2dGuiContext());


		debugRenderOverlay();
		vk2dRendererEndFrame();
	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	debugCleanup();
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}
