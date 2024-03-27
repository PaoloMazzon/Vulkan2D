#define SDL_MAIN_HANDLED
#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include "VK2D/Validation.h"
#include <stdio.h>
#include <time.h>
#include "../debug.c"


const vec2 POLY_1[] = {
        {13, 74},
        {99, 35},
        {152, 61},
        {151, 108},
        {74, 122},
};
const int POLY_1_COUNT = sizeof(POLY_1) / sizeof(vec2);

const vec2 POLY_2[] = {
        {216, 79},
        {230, 45},
        {331, 52},
        {365, 127},
        {363, 176},
        {293, 133},
};
const int POLY_2_COUNT = sizeof(POLY_2) / sizeof(vec2);

const vec2 POLY_3[] = {
        {340, 228},
        {361, 250},
        {332, 248},
};
const int POLY_3_COUNT = sizeof(POLY_3) / sizeof(vec2);

const vec2 POLY_4[] = {
        {232, 178},
        {285, 223},
        {275, 275},
        {140, 230},
        {139, 226},
};
const int POLY_4_COUNT = sizeof(POLY_4) / sizeof(vec2);

const vec2 POLY_5[] = {
        {42, 165},
        {88, 178},
        {93, 220},
        {22, 218},
};
const int POLY_5_COUNT = sizeof(POLY_5) / sizeof(vec2);

const int TOTAL_VERTEX_COUNT = POLY_1_COUNT + POLY_2_COUNT + POLY_3_COUNT + POLY_4_COUNT + POLY_5_COUNT;

/************************ Constants ************************/
const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;
const double MAX_VELOCITY = 2;
const double ACCELERATION = 0.2;
const double FRICTION = 0.5;

typedef struct {
    double x;
    double y;
} Point2D;

static inline double sign(double x) {
    return (x > 0 ? 1 : (x < 0 ? -1 : 0));
}

static inline double towardsZero(double num, double val) {
    double change = num > 0 ? -val : (num < 0 ? val : 0);
    if (sign(num) != sign(num + change)) {
        return -num;
    }
    return change;
}

static inline double clamp(double x, double min, double max) {
    return x > max ? max : (x < min ? min : x);
}

static inline double safeDiv(double numerator, double denominator) {
    if (denominator == 0)
        return numerator / (sign(denominator) * 0.0001);
    return numerator / denominator;
}

// Algorithm from https://ncase.me/sight-and-light/
static bool rayIntersection(Point2D *out, Point2D rayStart, Point2D rayEnd, Point2D segmentStart, Point2D segmentEnd) {
    double r_px = rayStart.x;
    double r_py = rayStart.y;
    double r_dx = rayEnd.x - rayStart.x;
    double r_dy = rayEnd.y - rayStart.y;

    double s_px = segmentStart.x;
    double s_py = segmentStart.y;
    double s_dx = segmentEnd.x - segmentStart.x;
    double s_dy = segmentEnd.y - segmentStart.y;

    double r_mag = sqrt(r_dx * r_dx + r_dy * r_dy);
    double s_mag = sqrt(s_dx * s_dx + s_dy * s_dy);
    if(r_dx / r_mag == s_dx / s_mag && r_dy / r_mag == s_dy / s_mag){
        return false;
    }

    double T2 = (r_dx * (s_py - r_py) + r_dy * (r_px - s_px)) / (s_dx * r_dy - s_dy * r_dx);
    double T1 = (s_px + s_dx * T2 - r_px) / r_dx;

    if(T1 < 0) return false;
    if(T2 < 0 || T2 > 1) return false;

    // Return the POINT OF INTERSECTION
    out->x = r_px + r_dx * T1;
    out->y = r_py + r_dy * T1;
    return true;
}

static inline double pointDistance(Point2D p1, Point2D p2) {
    return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
}

bool getPointIntersection(Point2D *out, vec2 *poly, int count, Point2D player, Point2D mouse) {
    Point2D closest = {0};
    bool started = false;
    for (int i = 0; i < count; i++) {
        Point2D p, segmentStart, segmentEnd;
        int startIndex = i == 0 ? count - 1 : i - 1;
        segmentStart.x = poly[startIndex][0];
        segmentStart.y = poly[startIndex][1];
        segmentEnd.x = poly[i][0];
        segmentEnd.y = poly[i][1];

        bool ray = rayIntersection(&p, player, mouse, segmentStart, segmentEnd);
        if (ray && (!started || pointDistance(player, p) < pointDistance(player, closest))) {
            closest = p;
            started = true;
        }
    }
    *out = closest;
    return started;
}

void addToShadowVertices(vec2 *poly, int count, Point2D player, Point2D mouse) {
    for (int i = 0; i < count; i++) {
        Point2D p, segmentStart, segmentEnd;
        int startIndex = i == 0 ? count - 1 : i - 1;
        segmentStart.x = poly[startIndex][0];
        segmentStart.y = poly[startIndex][1];
        segmentEnd.x = poly[i][0];
        segmentEnd.y = poly[i][1];

        bool ray = rayIntersection(&p, player, mouse, segmentStart, segmentEnd);
        if (ray) {
            vk2dRendererSetColourMod(VK2D_RED);
            vk2dDrawCircle(p.x, p.y, 2);
            vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);
        }
    }
}

int main(int argc, const char *argv[]) {
	// Basic SDL setup
	SDL_Window *window = SDL_CreateWindow("VK2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	SDL_Event e;
	bool quit = false;
	int keyboardSize;
	const uint8_t *keyboard = SDL_GetKeyboardState(&keyboardSize);
	if (window == NULL)
		return -1;

	// Initialize vk2d
	VK2DRendererConfig config = {VK2D_MSAA_32X, VK2D_SCREEN_MODE_IMMEDIATE, VK2D_FILTER_TYPE_NEAREST};
	vec4 clear = {0.0, 0.5, 1.0, 1.0};
	VK2DStartupOptions options = {true, true, true, "vk2derror.txt", false};
	int32_t error = vk2dRendererInit(window, config, &options);

	if (error < 0)
		return -1;

	VK2DCameraSpec defcam = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 0};
	vk2dRendererSetCamera(defcam);

	// Load Some test assets
	VK2DTexture playerTex = vk2dTextureLoad("assets/caveguy.png");

	VK2DCameraSpec cam = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f, 1, 0};
	VK2DCameraIndex testCamera = vk2dCameraCreate(cam);
	debugInit(window);

	// Framerate control
	double lastTime = SDL_GetPerformanceCounter();

	// Player
	double playerX = 120;
	double playerY = 120;
    double velocityX = 0;
    double velocityY = 0;
    double mouseX;
    double mouseY;

    // Polygons
    VK2DPolygon poly1 = vk2dPolygonCreate(POLY_1, POLY_1_COUNT);
    VK2DPolygon poly2 = vk2dPolygonCreate(POLY_2, POLY_2_COUNT);
    VK2DPolygon poly3 = vk2dPolygonCreate(POLY_3, POLY_3_COUNT);
    VK2DPolygon poly4 = vk2dPolygonCreate(POLY_4, POLY_4_COUNT);
    VK2DPolygon poly5 = vk2dPolygonCreate(POLY_5, POLY_5_COUNT);

    // Master vertex list
    VK2DVertexColour *shadowVertices = calloc(1, sizeof(VK2DVertexColour) * TOTAL_VERTEX_COUNT * 10);
    int shadowVertexCount = 0;

    while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
		}
		SDL_PumpEvents();

        // Adjust for window size
		int windowWidth, windowHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);
		cam.w = (float)windowWidth / 2;
		cam.h = (float)windowHeight / 2;
		vk2dCameraUpdate(testCamera, cam);
		int mx, my;
		SDL_GetMouseState(&mx, &my);
        mouseX = (double)mx * 0.5;
        mouseY = (double)my * 0.5;

        // Move player
        double moveX = (double)keyboard[SDL_SCANCODE_RIGHT] - (double)keyboard[SDL_SCANCODE_LEFT];
        double moveY = (double)keyboard[SDL_SCANCODE_DOWN] - (double)keyboard[SDL_SCANCODE_UP];
        velocityX += moveX != 0 ? moveX * ACCELERATION : towardsZero(velocityX, FRICTION);
        velocityY += moveY != 0 ? moveY * ACCELERATION : towardsZero(velocityY, FRICTION);
        velocityX = clamp(velocityX, -MAX_VELOCITY, MAX_VELOCITY);
        velocityY = clamp(velocityY, -MAX_VELOCITY, MAX_VELOCITY);
        playerX += velocityX;
        playerY += velocityY;
        playerX = playerX > windowWidth * 0.5 ? -vk2dTextureWidth(playerTex) : (playerX < -vk2dTextureWidth(playerTex) ? windowWidth * 0.5 : playerX);
        playerY = playerY > windowHeight * 0.5 ? -vk2dTextureHeight(playerTex) : (playerY < -vk2dTextureHeight(playerTex) ? windowHeight * 0.5 : playerY);

        // All rendering must happen after this
		vk2dRendererStartFrame(clear);

		// Lock framerate to 60
		vk2dSleep((((double)SDL_GetPerformanceFrequency() / 60) - ((double)SDL_GetPerformanceCounter() - lastTime)) / SDL_GetPerformanceFrequency());
        lastTime = SDL_GetPerformanceCounter();

		// Draw 2D portions
		vk2dRendererLockCameras(testCamera);
		vk2dRendererSetColourMod(VK2D_BLACK);
        vk2dDrawPolygon(poly1, 0, 0);
        vk2dDrawPolygon(poly2, 0, 0);
        vk2dDrawPolygon(poly3, 0, 0);
        vk2dDrawPolygon(poly4, 0, 0);
        vk2dDrawPolygon(poly5, 0, 0);
        vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);
        vk2dDrawTexture(playerTex, playerX, playerY);
        vk2dDrawCircle(mouseX, mouseY, 2);

        // Draw line intersection
        Point2D player = {playerX, playerY};
        Point2D mouse = {mouseX, mouseY};
        shadowVertexCount = 0;
        Point2D p;
        if (getPointIntersection(&p, POLY_1, POLY_1_COUNT, player, mouse)) {
            vk2dDrawCircle(p.x, p.y, 2);
        }
        if (getPointIntersection(&p, POLY_2, POLY_2_COUNT, player, mouse)) {
            vk2dDrawCircle(p.x, p.y, 2);
        }
        if (getPointIntersection(&p, POLY_3, POLY_3_COUNT, player, mouse)) {
            vk2dDrawCircle(p.x, p.y, 2);
        }
        if (getPointIntersection(&p, POLY_4, POLY_4_COUNT, player, mouse)) {
            vk2dDrawCircle(p.x, p.y, 2);
        }
        if (getPointIntersection(&p, POLY_5, POLY_5_COUNT, player, mouse)) {
            vk2dDrawCircle(p.x, p.y, 2);
        }
        //vk2dRendererDrawGeometry(shadowVertices, shadowVertexCount, 0, 0, true, 0, 1, 1, 0, 0, 0);

		debugRenderOverlay();

		// End the frame
		vk2dRendererEndFrame();
	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	debugCleanup();
	vk2dTextureFree(playerTex);
	free(shadowVertices);
    vk2dPolygonFree(poly1);
    vk2dPolygonFree(poly2);
    vk2dPolygonFree(poly3);
    vk2dPolygonFree(poly4);
    vk2dPolygonFree(poly5);
    vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}