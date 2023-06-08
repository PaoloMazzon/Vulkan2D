#define SDL_MAIN_HANDLED
#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include "VK2D/Validation.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include "../debug.c"

#define WAIT_ZERO(x) while (x != 0) {volatile int y = 0;}
#define WAIT_DIFF(x, y) while (x == y) {volatile int z = 0;}

// So each thread knows which pieces to update
typedef struct {
	int firstIndex; // First index in the master entity list this thread is supposed to deal with
	int count;      // Number of elements after firstIndex to touch
} ThreadData;

// Each entity is represented by one of these
typedef struct {
	float x;
	float y;
	float rot;
	vec4 colour;
	float scale;
} Entity;

/************************ Constants/Globals ************************/
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int BUFFER_COUNT = 2;
_Atomic bool gQuit = false;
_Atomic int gThreadsActive = 0;
_Atomic int gThreadsProcessingThisFrame = 0;
_Atomic int gCurrentBuffer = 0;
Entity *gEntities; // Entities every frame is built from
VK2DDrawInstance *gBuffers[] = {NULL, NULL};

static inline float clamp(float val, float min, float max) {
	if (val > max)
		return max;
	if (val < min)
		return min;
	return val;
}

void initializeEntity(Entity *e) {
	e->scale = vk2dRandom(0.5, 2);
	e->colour[0] = vk2dRandom(0.1, 1);
	e->colour[1] = vk2dRandom(0.1, 1);
	e->colour[2] = vk2dRandom(0.1, 1);
	e->colour[3] = 1;
	e->x = vk2dRandom(0, WINDOW_WIDTH);
	e->y = vk2dRandom(0, WINDOW_HEIGHT);
	e->rot = vk2dRandom(0, VK2D_PI * 2);
}

// Processes an entity, only touches the entity's data
void processEntity(Entity *e) {
	const float topSpeed = 1;
	const float topRotSpeed = 0.3;
	e->x += vk2dRandom(-topSpeed, topSpeed);
	e->y += vk2dRandom(-topSpeed, topSpeed);
	e->rot += vk2dRandom(-topRotSpeed, topRotSpeed);
}

// Turns an entity's data into instance data
void entityToInstance(Entity *e, VK2DDrawInstance *instance) {
	vk2dInstanceUpdate(instance, e->x, e->y, e->scale, e->scale, e->rot, 8, 8);
	instance->colour[0] = e->colour[0];
	instance->colour[1] = e->colour[1];
	instance->colour[2] = e->colour[2];
	instance->colour[3] = e->colour[3];
}

// Each thread
void *thread(void *data) {
	ThreadData *td = data;
	gThreadsActive += 1;

	while (!gQuit) {
		gThreadsProcessingThisFrame += 1;
		int currentBuffer = gCurrentBuffer;

		// Process this frame
		for (int i = td->firstIndex; i < td->firstIndex + td->count; i++) {
			processEntity(&gEntities[i]);
			entityToInstance(&gEntities[i], &gBuffers[gCurrentBuffer][i]);
		}

		gThreadsProcessingThisFrame -= 1;
		WAIT_DIFF(currentBuffer, gCurrentBuffer)
	}


	gThreadsActive -= 1;
	return NULL;
}

int main(int argc, const char *argv[]) {

	// Basic SDL setup
	SDL_Window *window = SDL_CreateWindow("VK2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	SDL_Event e;
	if (window == NULL)
		return -1;

	// Initialize vk2d
	VK2DRendererConfig config = {VK2D_MSAA_1X, VK2D_SCREEN_MODE_TRIPLE_BUFFER, VK2D_FILTER_TYPE_NEAREST};
	int pageSizeInMegabytes = 4; // This controls the number of instances in the demo
	VK2DStartupOptions options = {false, true, true, "vk2derror.txt", false, 1024 * 1024 * pageSizeInMegabytes};
	if (vk2dRendererInit(window, config, &options) < 0)
		return -1;

	// Delta and fps
	double lastTime = SDL_GetPerformanceCounter();
	double delta = 0;
	double time = 0;

	// Load assets
	debugInit(window);
	VK2DTexture texCaveguy = vk2dTextureLoad("assets/caveguy.png");

	// Create cameras
	VK2DCameraSpec cameraSpec = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
	VK2DCameraIndex cameraIndex = vk2dCameraCreate(cameraSpec);

	// Make a crazy number of instances
	VK2DRendererLimits limits = vk2dRendererGetLimits();
	// if we make it the limit it will take a whole other page to store all the
	// instance data because the two cameras already consume some of the vram
	// pages so we subtract a few instances. This doesn't really matter when we
	// only lose out on 12ish megabytes of vram/ram but if we want millions of
	// instances we can start losing out on gigabytes.
	const int instanceCount = limits.maxInstancedDraws - 100;
	printf("Caveguys: %i\n", instanceCount);
	fflush(stdout);
	gEntities = malloc(sizeof(Entity) * instanceCount);
	for (int i = 0; i < instanceCount; i++) {
		initializeEntity(&gEntities[i]);
	}
	for (int i = 0; i < BUFFER_COUNT; i++) {
		gBuffers[i] = malloc(sizeof(VK2DDrawInstance) * instanceCount);
		for (int j = 0; j < instanceCount; j++) {
			gBuffers[i][j].texturePos[0] = 0;
			gBuffers[i][j].texturePos[1] = 0;
			gBuffers[i][j].texturePos[2] = 16;
			gBuffers[i][j].texturePos[3] = 16;
		}
	}

	// Initialize threads
	const int THREAD_COUNT = SDL_GetCPUCount() - 1;
	pthread_t threads[THREAD_COUNT];
	ThreadData threadData[THREAD_COUNT];

	// Build thread data
	int chunkSize = instanceCount / THREAD_COUNT;
	int leftOver = instanceCount % THREAD_COUNT;
	int pos = 0;
	for (int i = 0; i < THREAD_COUNT; i++) {
		threadData[i].count = chunkSize;
		if (leftOver > 0) {
			threadData[i].count += 1;
			leftOver -= 1;
		}
		threadData[i].firstIndex = pos;
		pos += threadData[i].count;
	}

	// Start threads
	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&threads[i], &attr, thread, &threadData[i]);
	}

	while (!gQuit) {
		delta = ((double)SDL_GetPerformanceCounter() - lastTime) / (double)SDL_GetPerformanceFrequency();
		lastTime = SDL_GetPerformanceCounter();
		time += delta;

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				gQuit = true;
			}
		}
		SDL_PumpEvents();

		// Scale camera
		int ww, wh;
		SDL_GetWindowSize(window, &ww, &wh);
		float w = (float)ww;
		float h = (float)wh;
		cameraSpec.wOnScreen = w;
		cameraSpec.hOnScreen = h;
		cameraSpec.h = (h / w) * (float)WINDOW_WIDTH;
		vk2dCameraUpdate(cameraIndex, cameraSpec);

		// All rendering must happen after this
		vk2dRendererStartFrame(VK2D_BLACK);

		// Draw game
		WAIT_ZERO(gThreadsProcessingThisFrame)
		int buffer = gCurrentBuffer;
		// Move onto next buffer
		if (gCurrentBuffer == BUFFER_COUNT - 1) {
			gCurrentBuffer = 0;
		} else {
			gCurrentBuffer += 1;
		}

		vk2dRendererLockCameras(cameraIndex);
		vk2dRendererDrawInstanced(texCaveguy, gBuffers[buffer], instanceCount);
		vk2dRendererUnlockCameras();

		debugRenderOverlay();

		// End the frame
		vk2dRendererEndFrame();
	}

	WAIT_ZERO(gThreadsActive)

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	vk2dTextureFree(texCaveguy);
	debugCleanup();
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	for (int i = 0; i < BUFFER_COUNT; i++)
		free(gBuffers[i]);
	free(gEntities);
	return 0;
}