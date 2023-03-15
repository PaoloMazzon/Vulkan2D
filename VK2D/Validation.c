/// \file Validation.c
/// \author Paolo Mazzon
#include "VK2D/Validation.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>
#include <assert.h>
#include "VK2D/Renderer.h"

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
		vprintf(fmt, list);
		printf("\n");
		fflush(stdout);
		va_end(list);
	} else {
		va_list list;
		va_start(list, fmt);
		va_end(list);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL _vk2dDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t sourceObject, size_t location, int32_t messageCode, const char* layerPrefix, const char* message, void* data) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	FILE* output;
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		output = stderr;
	else
		output = stdout;
	fprintf(output, "[VK2D] Callback: %i/%i/%" PRIu64 "/%" PRIu64 "/%i (%s) %s\n", flags, objectType, sourceObject, (uint64_t)location, messageCode, layerPrefix, message);
	fflush(output);
	if (gRenderer->options.errorFile != NULL) {
		FILE *file = fopen(gRenderer->options.errorFile, "a");
		fprintf(file, "[VK2D] Callback: %i/%i/%" PRIu64 "/%" PRIu64 "/%i (%s) %s\n", flags, objectType, sourceObject,
				(uint64_t) location, messageCode, layerPrefix, message);
		fclose(file);
	}
	if (gRenderer->options.quitOnError)
		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
			abort();
	return false;
}