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

const vec2 ROOM[] = {
        {0, 0},
        {400, 0},
        {400, 300},
        {0, 300},
};
const int ROOM_COUNT = sizeof(ROOM) / sizeof(vec2);

const int TOTAL_VERTEX_COUNT = ROOM_COUNT + POLY_1_COUNT + POLY_2_COUNT + POLY_3_COUNT + POLY_4_COUNT + POLY_5_COUNT + 4;

const vec2 *POLY_LIST[] = {POLY_1, POLY_2, POLY_3, POLY_4, POLY_5, ROOM};
const int POLY_COUNT_LIST[] = {POLY_1_COUNT, POLY_2_COUNT, POLY_3_COUNT, POLY_4_COUNT, POLY_5_COUNT, ROOM_COUNT};
const int POLY_COUNT = 6;

/************************ Constants ************************/
const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;
const double MAX_VELOCITY = 2;
const double ACCELERATION = 0.2;
const double FRICTION = 0.5;
const double FAR_AWAY = 3000;

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

bool getClosestIntersection(Point2D *out, Point2D player, Point2D mouse) {
    Point2D closest = {0};
    bool started = false;

    for (int i = 0; i < POLY_COUNT; i++) {
        Point2D p;
        if (getPointIntersection(&p, POLY_LIST[i], POLY_COUNT_LIST[i], player, mouse)) {
            if (!started || pointDistance(player, p) < pointDistance(player, closest)) {
                started = true;
                closest = p;
            }
        }
    }

    // Account for no point
    if (!started) {
        double m = atan2(mouse.y - player.y, mouse.x - player.x);
        closest.x = player.x + (FAR_AWAY * cos(m));
        closest.y = player.y + (FAR_AWAY * sin(m));
    }

    *out = closest;
    return started;
}

void addVertex(VK2DVertexColour *vertices, int *index, Point2D p) {
    vertices[*index].pos[0] = p.x;
    vertices[(*index)++].pos[1] = p.y;
}

int addToShadowVertices(vec2 *allVertices, VK2DVertexColour *vertices, Point2D player) {
    int vertexIndex = 0;
    bool usePrev = false;
    Point2D previous = {0};
    for (int i = 0; i < TOTAL_VERTEX_COUNT; i++) {
        double rads = atan2(allVertices[i][1] - player.y, allVertices[i][0] - player.x);
        double radMove = 0.00001;
        Point2D dest = {player.x + (10 * cos(rads)), player.y + (10 * sin(rads))};
        Point2D destLeft = {player.x + (10 * cos(rads - radMove)), player.y + (10 * sin(rads - radMove))};
        Point2D destRight = {player.x + (10 * cos(rads + radMove)), player.y + (10 * sin(rads + radMove))};
        Point2D d, dl, dr;

        getClosestIntersection(&d, player, dest);
        getClosestIntersection(&dl, player, destLeft);
        getClosestIntersection(&dr, player, destRight);

        addVertex(vertices, &vertexIndex, player);
        addVertex(vertices, &vertexIndex, previous);
        addVertex(vertices, &vertexIndex, dl);
        addVertex(vertices, &vertexIndex, player);
        addVertex(vertices, &vertexIndex, player);
        addVertex(vertices, &vertexIndex, dl);
        addVertex(vertices, &vertexIndex, d);
        addVertex(vertices, &vertexIndex, player);
        addVertex(vertices, &vertexIndex, player);
        addVertex(vertices, &vertexIndex, d);
        addVertex(vertices, &vertexIndex, dr);
        previous = dr;
        usePrev = true;

    }
    return vertexIndex;
}

void swap(vec2 a, vec2 b) {
    vec2 temp = {a[0], a[1]};
    a[0] = b[0];
    a[1] = b[1];
    b[0] = temp[0];
    b[1] = temp[1];
}

void swapf(double* a, double* b) {
    double temp = *a;
    *a = *b;
    *b = temp;
}

void quickSort(vec2 arr[], double comp[], int low, int high) {
    if (low < high) {
        double pivot = comp[high];
        int i = low - 1;

        for (int j = low; j <= high - 1; j++) {
            if (comp[j] < pivot) {
                i++;
                swap(arr[i], arr[j]);
                swapf(&comp[i], &comp[j]);
            }
        }
        swap(arr[i + 1], arr[high]);
        swapf(&comp[i + 1], &comp[high]);
        int pi = i + 1;

        quickSort(arr, comp, low, pi - 1);
        quickSort(arr, comp, pi + 1, high);
    }
}

// Sorts all the vertices by their angle relative to pivot clockwise
void buildVertexList(vec2 *vertices, int count, Point2D pivot) {
    double *angles = malloc(count * sizeof(double));

    // Find all angles and sort
    for (int i = 0; i < count; i++)
        angles[i] = atan2(vertices[i][1] - pivot.y, vertices[i][0] - pivot.x);
    quickSort(vertices, angles, 0, count - 1);

    free(angles);
}

int main(int argc, const char *argv[]) {
	// Basic SDL setup
	SDL_Window *window = SDL_CreateWindow("VK2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
	SDL_Event e;
	bool quit = false;
	int keyboardSize;
	const uint8_t *keyboard = SDL_GetKeyboardState(&keyboardSize);
	if (window == NULL)
		return -1;

	// Initialize vk2d
	VK2DRendererConfig config = {VK2D_MSAA_1X, VK2D_SCREEN_MODE_IMMEDIATE, VK2D_FILTER_TYPE_NEAREST};
	vec4 clear = {0.0, 0.5, 1.0, 1.0};
	VK2DStartupOptions options = {true, true, true, "vk2derror.txt", false};
	int32_t error = vk2dRendererInit(window, config, &options);

	if (error < 0)
		return -1;

	VK2DCameraSpec defcam = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 0};
	vk2dRendererSetCamera(defcam);

	// Load Some test assets
	VK2DTexture playerTex = vk2dTextureLoad("assets/caveguy.png");
	VK2DTexture shadowsTex = vk2dTextureCreate(400, 300);

	VK2DCameraSpec cam = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f, 1, 0};
	VK2DCameraIndex testCamera = vk2dCameraCreate(cam);
	debugInit(window);

	// Framerate control
	double lastTime = SDL_GetPerformanceCounter();

	// Player
	double playerX = 140;
	double playerY = 140;
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
    VK2DVertexColour *shadowVertices = calloc(1, sizeof(VK2DVertexColour) * 1024);
    for (int i = 0; i < TOTAL_VERTEX_COUNT * 10; i++) {
        shadowVertices[i].colour[0] = 1;
        shadowVertices[i].colour[1] = 1;
        shadowVertices[i].colour[2] = 1;
        shadowVertices[i].colour[3] = 1;
    }
    int shadowVertexCount = 0;

    // Assemble array of all vertices
    vec2 *allVertices = malloc(sizeof(vec2) * TOTAL_VERTEX_COUNT);
    int v = 0;
    for (int i = 0; i < ROOM_COUNT; i++) {
        allVertices[v][0] = ROOM[i][0];
        allVertices[v++][1] = ROOM[i][1];
    }
    for (int i = 0; i < POLY_1_COUNT; i++) {
        allVertices[v][0] = POLY_1[i][0];
        allVertices[v++][1] = POLY_1[i][1];
    }
    for (int i = 0; i < POLY_2_COUNT; i++) {
        allVertices[v][0] = POLY_2[i][0];
        allVertices[v++][1] = POLY_2[i][1];
    }
    for (int i = 0; i < POLY_3_COUNT; i++) {
        allVertices[v][0] = POLY_3[i][0];
        allVertices[v++][1] = POLY_3[i][1];
    }
    for (int i = 0; i < POLY_4_COUNT; i++) {
        allVertices[v][0] = POLY_4[i][0];
        allVertices[v++][1] = POLY_4[i][1];
    }
    for (int i = 0; i < POLY_5_COUNT; i++) {
        allVertices[v][0] = POLY_5[i][0];
        allVertices[v++][1] = POLY_5[i][1];
    }

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
        vk2dDrawTexture(playerTex, playerX - (vk2dTextureWidth(playerTex) / 2), playerY - (vk2dTextureHeight(playerTex) / 2));
        vk2dDrawCircle(mouseX, mouseY, 2);

        // Draw shadows
        Point2D player = {playerX, playerY};
        shadowVertexCount = 0;
        buildVertexList(allVertices, TOTAL_VERTEX_COUNT, player);
        shadowVertexCount = addToShadowVertices(allVertices, shadowVertices, player);
        /*vk2dRendererSetTarget(shadowsTex);
        vk2dRendererSetColourMod(VK2D_BLACK);
        vk2dRendererClear();
        vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);
        vk2dRendererSetBlendMode(VK2D_BLEND_MODE_SUBTRACT);*/
        vk2dRendererDrawGeometry(shadowVertices, shadowVertexCount, 0, 0, true, 1, 1, 1, 0, 0, 0);
        /*vk2dRendererSetBlendMode(VK2D_BLEND_MODE_BLEND);
        vk2dRendererSetTarget(VK2D_TARGET_SCREEN);*/
        vk2dDrawTexture(shadowsTex, 0, 0);

		debugRenderOverlay();

		// End the frame
		vk2dRendererEndFrame();
	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	debugCleanup();
	vk2dTextureFree(shadowsTex);
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