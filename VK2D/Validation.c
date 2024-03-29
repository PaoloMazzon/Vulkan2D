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

void vk2dValidationBegin() {
	gLogMutex = SDL_CreateMutex();
}

void vk2dValidationEnd() {
	SDL_DestroyMutex(gLogMutex);
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

void vk2dLogMessage(const char* fmt, ...) {
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