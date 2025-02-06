/// \file debug.c
/// \author Paolo Mazzon
/// \brief This is meant to be included once in each example project for the overlay
#include <VK2D/VK2D.h>

static VK2DTexture gDebugFont = NULL;
static SDL_Window *gWindow;

void debugInit(SDL_Window *window) {
	gWindow = window;
	gDebugFont = vk2dTextureLoad("assets/font.png");
}

// Very basic and simple font renderer for the font in this test specifically
void debugRenderFont(float x, float y, const char *text) {
	float ox = x;
	for (int i = 0; i < (int)strlen(text); i++) {
		if (text[i] != '\n') {
			vk2dRendererDrawTexture(gDebugFont, x, y, 2, 2, 0, 0, 0, (text[i] * 8) % 128, floorf(text[i] / 16) * 16, 8, 16);
			x += 8 * 2;
		} else {
			x = ox;
			y += 16 * 2;
		}
	}
}

void debugRenderOverlay() {
	vk2dRendererLockCameras(VK2D_DEFAULT_CAMERA);
	char title[100];
	VK2DRendererConfig conf = vk2dRendererGetConfig();
	float inUse, total;
	vk2dRendererGetVRAMUsage(&inUse, &total);
	sprintf(title, "Vulkan2D [%0.2fms] [%0.2ffps] %ix MSAA\nVRAM: %0.2fMiB/%0.2fGiB", vk2dRendererGetAverageFrameTime(), 1000 / vk2dRendererGetAverageFrameTime(), conf.msaa, inUse, total / 1024);
	vk2dRendererSetColourMod(VK2D_BLACK);
	int w, h;
	SDL_GetWindowSize(gWindow, &w, &h);
	vk2dDrawRectangle(0, 0, (float)w, 17*2*2);
	vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);
	debugRenderFont(0, 0, title);
	vk2dRendererUnlockCameras();
}

void debugCleanup() {
	vk2dTextureFree(gDebugFont);
	gDebugFont = NULL;
}

const VK2DVertexColour SAMPLE_TRIANGLE[] = {
		{{0.0, 0.0, +0.0}, {1.0, 1.0, 0.5, 1}},
		{{400, 300, +0.0}, {1.0, 0.5, 1.0, 1}},
		{{400, 0.0, +0.0}, {0.5, 1.0, 1.0, 1}},
		{{0.0, 0.0, +0.0}, {0.5, 0.5, 1.0, 1}},
		{{0.0, 300, +0.0}, {0.0, 1.0, 0.5, 1}},
		{{400, 300, +0.0}, {1.0, 1.0, 1.0, 1}}
};
const uint32_t SAMPLE_TRIANGLE_VERTICES = 6;

const VK2DVertexColour SAMPLE_RECTANGLE[] = {
        {{0.0, 0.0, +0.0}, {1.0, 1.0, 0.5, 1}},
        {{400, 0.0, +0.0}, {1.0, 0.5, 1.0, 1}},
        {{400, 300, +0.0}, {0.5, 1.0, 1.0, 1}},
        {{0.0, 300, +0.0}, {0.5, 0.5, 1.0, 1}},
        {{0.0, 0.0, +0.0}, {1.0, 1.0, 0.5, 1}}
};
const uint32_t SAMPLE_RECTANGLE_VERTICES = 5;