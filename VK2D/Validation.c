/// \file Validation.c
/// \author Paolo Mazzon
#include "VK2D/Validation.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>
#include <assert.h>
#include "VK2D/Renderer.h"
#include "VK2D/Opaque.h"

static SDL_mutex *gLogMutex = NULL;

// Global log buffer
static const int32_t gLogBufferSize = 4096;
static char gLogBuffer[4096] = "Vulkan2D is not initialized.";
static VK2DStatus gStatus;
static bool gResetLog;         // So the raise method knows to reset after the user gets the output
static const char *gErrorFile; // Makes sure this file has access to the error log file immediately
static bool gQuitOnError;      // Same as above

void vk2dValidationBegin(const char *errorFile, bool quitOnError) {
	gLogMutex = SDL_CreateMutex();
	gErrorFile = errorFile;
	gQuitOnError = quitOnError;
	gLogBuffer[0] = 0;
	gStatus = 0;
}

void vk2dValidationEnd() {
	SDL_DestroyMutex(gLogMutex);
	strncpy(gLogBuffer, "Vulkan2D is not initialized.", gLogBufferSize);
}

bool _vk2dErrorRaise(VkResult result, const char* function, int line, const char* varname) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	fprintf(stderr, "[VK2D]: Error %i occurred in function \"%s\" line %i: %s\n", result, function, line, varname);
	fflush(stderr);
	if (gRenderer->options.errorFile != NULL) {
		FILE *file = fopen(gRenderer->options.errorFile, "a");
		fprintf(file, "[VK2D]: Error %i occurred in function \"%s\" line %i: %s\n", result, function, line, varname);
		fclose(file);
	}
	if (gRenderer->options.quitOnError)
		abort();
	return false;
}

bool _vk2dPointerCheck(void* ptr, const char* function, int line, const char* varname) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (ptr == NULL) {
		fprintf(stderr, "[VK2D]: Pointer check failed for \"%s\" in function \"%s\" on line %i\n", varname, function, line);
		fflush(stderr);
		if (gRenderer->options.errorFile != NULL) {
			FILE *file = fopen(gRenderer->options.errorFile, "a");
			fprintf(file, "[VK2D]: Pointer check failed for \"%s\" in function \"%s\" on line %i\n", varname, function,
					line);
			fclose(file);
		}
		if (gRenderer->options.quitOnError)
			abort();
	}
	return 1;
}

// Safe string length method
static int32_t stringLength(const char *str, int32_t size) {
    int32_t len = 0;
    for (int32_t i = 0; str[i] != 0 && i < size; i++) {
        len++;
    }
    return len;
}

void vk2dRaise(VK2DStatus result, const char* fmt, ...) {
    // Ignore double raises on "renderer not intialized"
    if (result == VK2D_STATUS_RENDERER_NOT_INITIALIZED && (gStatus & VK2D_STATUS_RENDERER_NOT_INITIALIZED) != 0)
        return;

    // Reset if the user got the log recently
    if (gResetLog) {
        gResetLog = false;
        gStatus = 0;
        gLogBuffer[0] = 0;
    }
    gStatus = gStatus | result;

    // Calculate what part of the buffer to write to
    const int startIndex = stringLength(gLogBuffer, gLogBufferSize);
    const int32_t length = gLogBufferSize - startIndex;

    // Print output
    va_list list;
    va_start(list, fmt);
    vsnprintf(&gLogBuffer[startIndex], length, fmt, list);
    va_end(list);

    // Print output to file
    if (gErrorFile != NULL) {
        FILE *f = fopen(gErrorFile, "a");
        if (f != NULL) {
            va_start(list, fmt);
            vfprintf(f, fmt, list);
            fprintf(f, "\n");
            va_end(list);
            fclose(f);
        }
    }

    // Crash if thats the user option
    if (gQuitOnError && vk2dStatusFatal()) {
        abort();
    }
}

VK2DStatus vk2dStatus() {
    gResetLog = true;
    return gStatus;
}

bool vk2dStatusFatal() {
    return (gStatus & ~(VK2D_STATUS_NONE | VK2D_STATUS_SDL_ERROR | VK2D_STATUS_FILE_NOT_FOUND | VK2D_STATUS_BAD_ASSET)) != 0;
}

const char *vk2dStatusMessage() {
    gResetLog = true;
    return gLogBuffer;
}

void vk2dLog(const char* fmt, ...) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->options.stdoutLogging) {
		va_list list;
		va_start(list, fmt);
		SDL_LockMutex(gLogMutex);
		vprintf(fmt, list);
		printf("\n");
		fflush(stdout);
		SDL_UnlockMutex(gLogMutex);
		va_end(list);
	} else {
		va_list list;
		va_start(list, fmt);
		va_end(list);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL _vk2dDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t sourceObject, size_t location, int32_t messageCode, const char* layerPrefix, const char* message, void* data) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	char firstHalf[1000];
	FILE* output;
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		output = stderr;
	else
		output = stdout;
	snprintf(firstHalf, 999, "%s", layerPrefix);
	fprintf(output, "%s\n", firstHalf);
	for (int i = 0; i < strlen(firstHalf); i++)
		fprintf(output, "-");
	fprintf(output, "\n%s\n", message);
	fflush(output);
	if (gRenderer->options.errorFile != NULL) {
		FILE *file = fopen(gRenderer->options.errorFile, "a");
		fprintf(file, "%s\n", firstHalf);
		for (int i = 0; i < strlen(firstHalf); i++)
			fprintf(file, "-");
		fprintf(file, "\n%s\n", message);
		fclose(file);
	}
	if (gRenderer->options.quitOnError)
		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
			abort();
	return false;
}