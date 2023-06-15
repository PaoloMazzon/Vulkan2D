#define SDL_MAIN_HANDLED
#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include "VK2D/Validation.h"
#include <stdio.h>
#include <time.h>
#include "../debug.c"

/************************ Constants ************************/
const float LOGICAL_WIDTH = 256;
const float LOGICAL_HEIGHT = 224;
const int WINDOW_WIDTH  = (int)LOGICAL_WIDTH * 3;
const int WINDOW_HEIGHT = (int)LOGICAL_HEIGHT * 3;

static inline float min(float x, float y) {
	if (x < y)
		return x;
	return y;
}

int main(int argc, const char *argv[]) {
	// Basic SDL setup
	SDL_Window *window = SDL_CreateWindow("VK2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	SDL_Event e;
	bool quit = false;
	if (window == NULL)
		return -1;

	// Initialize vk2d
	VK2DRendererConfig config = {VK2D_MSAA_1X, VK2D_SCREEN_MODE_TRIPLE_BUFFER, VK2D_FILTER_TYPE_NEAREST};
	vec4 clear = {0.1, 0.0, 0.2, 1.0};
	VK2DStartupOptions options = {true, true, true, "vk2derror.txt", false};
	if (vk2dRendererInit(window, config, &options) < 0)
		return -1;

	// Delta and fps
	double lastTime = SDL_GetPerformanceCounter();
	double delta = 0;
	double time = 0;

	// Queue off-thread loading
	VK2DTexture texCaveGuy = NULL;
	VK2DTexture texCaveGuyUV = NULL;
	VK2DModel modelCaveGuy = NULL;
	VK2DShader shaderShiny = NULL;
	const int ASSET_COUNT = 4;
	VK2DAssetLoad loads[4] = {0};
	loads[0].type = VK2D_ASSET_TYPE_MODEL_FILE;
	loads[0].Load.filename = "assets/caveguydie.obj";
	loads[0].Data.Model.tex = &texCaveGuyUV;
	loads[0].Output.model = &modelCaveGuy;
	loads[1].type = VK2D_ASSET_TYPE_TEXTURE_FILE;
	loads[1].Load.filename = "assets/caveguyuv.png";
	loads[1].Output.texture = &texCaveGuyUV;
	loads[2].type = VK2D_ASSET_TYPE_TEXTURE_FILE;
	loads[2].Load.filename = "assets/caveguy.png";
	loads[2].Output.texture = &texCaveGuy;
	loads[3].type = VK2D_ASSET_TYPE_SHADER_FILE;
	loads[3].Load.filename = "assets/tex.vert.spv";
	loads[3].Load.fragmentFilename = "assets/tex.frag.spv";
	loads[3].Data.Shader.uniformBufferSize = 4;
	loads[3].Output.shader = &shaderShiny;
	vk2dAssetsLoad(loads, ASSET_COUNT);

	// Load loading screen
	debugInit(window);
	VK2DTexture texTarget = vk2dTextureCreate(LOGICAL_WIDTH, LOGICAL_HEIGHT);
	VK2DTexture texLoading = vk2dTextureLoad("assets/loading.png");

	// Moving caveguy
	float x = LOGICAL_WIDTH / 2;
	float y = LOGICAL_HEIGHT / 2;
	float dir = VK2D_PI / 4;
	const float speed = 70;
	const float scale = 3;

	while (!quit) {
		delta = ((double)SDL_GetPerformanceCounter() - lastTime) / (double)SDL_GetPerformanceFrequency();
		lastTime = SDL_GetPerformanceCounter();
		time += delta;

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
		}
		SDL_PumpEvents();

		// All rendering must happen after this
		vk2dRendererStartFrame(VK2D_BLACK);

		// Clear the virtual target and start rendering to it
		vk2dRendererSetTarget(texTarget);
		vk2dRendererSetColourMod(clear);
		vk2dRendererClear();
		vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);

		// Either draw loading screen or the demo
		if (vk2dAssetsLoadComplete()) {
			// Loading is complete

			// Move the caveguy
			x += cos(dir) * speed * delta;
			y += sin(dir) * speed * delta;
			if (x + (8 * scale) > LOGICAL_WIDTH || y + (8 * scale) > LOGICAL_HEIGHT || x - (8 * scale) < 0 || y - (8 * scale) < 0)
				dir -= (VK2D_PI / 4);
			vk2dDrawTextureExt(texCaveGuy, x - (8 * scale), y - (8 * scale), scale, scale, -time, 8, 8);
		} else {
			// Loading is not complete
			vk2dDrawTexture(texLoading, (LOGICAL_WIDTH / 2) - (vk2dTextureWidth(texLoading) / 2), (LOGICAL_HEIGHT / 2) - (vk2dTextureHeight(texLoading) / 2));
			vk2dRendererSetColourMod(VK2D_BLACK);
			vk2dDrawRectangleOutline((LOGICAL_WIDTH / 2) - 30, (LOGICAL_HEIGHT / 2) + 10, 60, 8, 1);
			vk2dDrawRectangle((LOGICAL_WIDTH / 2) - 29, (LOGICAL_HEIGHT / 2) + 11, 57 * vk2dAssetsLoadStatus(), 5);
			vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);
		}

		vk2dRendererSetTarget(VK2D_TARGET_SCREEN);

		// Draw the virtual target integer-scaled to the center of the window
		int ww, wh;
		SDL_GetWindowSize(window, &ww, &wh);
		float w = (float)ww;
		float h = (float)wh;
		float scale = min(floorf(w / LOGICAL_WIDTH), floorf(h / LOGICAL_WIDTH)) + 1;
		float xOnWindow = (w - (LOGICAL_WIDTH * scale)) / 2;
		float yOnWindow = (h - (LOGICAL_HEIGHT * scale)) / 2;
		vk2dRendererLockCameras(VK2D_DEFAULT_CAMERA);
		vk2dDrawTextureExt(texTarget, xOnWindow, yOnWindow, scale, scale, 0, 0, 0);

		debugRenderOverlay();

		// End the frame
		vk2dRendererEndFrame();
	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	vk2dTextureFree(texTarget);
	vk2dTextureFree(texLoading);
	vk2dAssetsFree(loads, ASSET_COUNT);
	debugCleanup();
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}