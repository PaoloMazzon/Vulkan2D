#define SDL_MAIN_HANDLED
#include <SDL3/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include "VK2D/Validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "../debug.c"

/************************ Constants ************************/

const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;

unsigned char* _vk2dLoadFile(const char *filename, uint32_t *size);
bool _vk2dShaderCompile(const char *shader, uint32_t shaderSize, void *compiledShaders);
int main(int argc, const char *argv[]) {
	// Basic SDL setup
    SDL_Init(SDL_INIT_EVENTS);
	SDL_Window *window = SDL_CreateWindow("VK2D", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	SDL_Event e;
	bool quit = false;
	int keyboardSize;
	const bool *keyboard = SDL_GetKeyboardState(&keyboardSize);
	if (window == NULL)
		return -1;

	// Initialize vk2d
	VK2DRendererConfig config = {VK2D_MSAA_1X, VK2D_SCREEN_MODE_IMMEDIATE, VK2D_FILTER_TYPE_NEAREST};
	vec4 clear = {0.0, 0.5, 1.0, 1.0};
	VK2DStartupOptions options = {
	        .quitOnError = true,
	        .enableDebug = false,
	        .stdoutLogging = true,
	        .vramPageSize = sizeof(VK2DDrawInstance) * 2000010,
	};
	vk2dRendererInit(window, config, &options);
    debugInit(window);

	// Load Some test assets
	VK2DTexture texCaveguy = vk2dTextureLoad("assets/caveguy.png");

	// A texture whose pixels are replaced every frame, like a video frame
	enum {DYNAMIC_SIZE = 64, PATCH_SIZE = 16};
	static uint8_t dynamicPixels[DYNAMIC_SIZE * DYNAMIC_SIZE * 4];
	static uint8_t patchPixels[PATCH_SIZE * PATCH_SIZE * 4];
	for (int i = 0; i < PATCH_SIZE * PATCH_SIZE; i++) {
		patchPixels[i * 4 + 0] = 255;
		patchPixels[i * 4 + 1] = 255;
		patchPixels[i * 4 + 2] = 255;
		patchPixels[i * 4 + 3] = 255;
	}
	VK2DImage dynamicImage = vk2dImageFromPixels(vk2dRendererGetDevice(), dynamicPixels, DYNAMIC_SIZE, DYNAMIC_SIZE, true);
	VK2DTexture texDynamic = vk2dTextureLoadFromImage(dynamicImage);

	// Delta and fps
	const double startTime = SDL_GetPerformanceCounter();

    VK2DShader testShader = vk2dSlangLoad("assets/shader.slang");
    if (!testShader) {
        abort();
    }

	while (!quit && !vk2dStatusFatal()) {
		const double time = (double)(SDL_GetPerformanceCounter() - startTime) / (double)SDL_GetPerformanceFrequency();

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) {
				quit = true;
			}
		}
		SDL_PumpEvents();
		int windowWidth, windowHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);

        // Test shader
        float val = vk2dTime() * 5;
        vk2dRendererDrawShader(testShader, &val,
                               texCaveguy,
                               150, 150,
                               10, 10,
                               0, 0, 0,
                               0, 0, 16, 16);

		// Rewrite the whole dynamic texture, then a moving patch of it
		for (int y = 0; y < DYNAMIC_SIZE; y++) {
			for (int x = 0; x < DYNAMIC_SIZE; x++) {
				uint8_t *p = &dynamicPixels[(y * DYNAMIC_SIZE + x) * 4];
				p[0] = (uint8_t)(128 + 127 * sin(x * 0.2 + time * 3));
				p[1] = (uint8_t)(128 + 127 * sin(y * 0.2 + time * 2));
				p[2] = (uint8_t)(128 + 127 * sin((x + y) * 0.1 + time));
				p[3] = 255;
			}
		}
		vk2dTextureUpdate(texDynamic, dynamicPixels);
		int patchX = (int)((DYNAMIC_SIZE - PATCH_SIZE) * (0.5 + 0.5 * sin(time)));
		vk2dTextureUpdateRegion(texDynamic, patchPixels, patchX, 8, PATCH_SIZE, PATCH_SIZE);
		vk2dRendererDrawTexture(texDynamic, 400, 150, 4, 4, 0, 0, 0, 0, 0, DYNAMIC_SIZE, DYNAMIC_SIZE);

		debugRenderOverlay();

        vk2dRendererPresent();
	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
    vk2dShaderFree(testShader);
	vk2dTextureFree(texCaveguy);
	vk2dTextureFree(texDynamic);
	vk2dImageFree(dynamicImage);
	debugCleanup();
    vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}
