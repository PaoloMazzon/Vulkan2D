/// \file Validation.c
/// \author Paolo Mazzon
#include "VK2D/Validation.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>
#include "VK2D/BuildOptions.h"
#include <assert.h>

bool _vk2dErrorRaise(VkResult result, const char* function, int line, const char* varname) {
	fprintf(stderr, "[VK2D]: Error %i occurred in function \"%s\" line %i: %s\n", result, function, line, varname);
	fflush(stderr);
#ifdef VK2D_ERROR_FILE
	FILE *file = fopen(VK2D_ERROR_FILE, "a");
	fprintf(file, "[VK2D]: Error %i occurred in function \"%s\" line %i: %s\n", result, function, line, varname);
	fclose(file);
#endif // VK2D_ERROR_FILE
#ifdef VK2D_QUIT_ON_ERROR
		abort();
#endif //VK2D_QUIT_ON_ERROR
	return false;
}

bool _vk2dPointerCheck(void* ptr, const char* function, int line, const char* varname) {
	if (ptr == NULL) {
		fprintf(stderr, "[VK2D]: Pointer check failed for \"%s\" in function \"%s\" on line %i\n", varname, function, line);
		fflush(stderr);
#ifdef VK2D_ERROR_FILE
		FILE *file = fopen(VK2D_ERROR_FILE, "a");
		fprintf(file, "[VK2D]: Pointer check failed for \"%s\" in function \"%s\" on line %i\n", varname, function, line);
		fclose(file);
#endif // VK2D_ERROR_FILE
#ifdef VK2D_QUIT_ON_ERROR
		abort();
#endif //VK2D_QUIT_ON_ERROR
		return 0;
	}
	return 1;
}

void vk2dLogMessage(const char* fmt, ...) {
#ifdef VK2D_STDOUT_LOGGING
	va_list list;
	va_start(list, fmt);
	vprintf(fmt, list);
	printf("\n");
	fflush(stdout);
	va_end(list);
#else // VK2D_STDOUT_LOGGING
	va_list list;
	va_start(list, fmt);
	va_end(list);
#endif // VK2D_STDOUT_LOGGING
}

VKAPI_ATTR VkBool32 VKAPI_CALL _vk2dVulkanRenderEngineDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t sourceObject, size_t location, int32_t messageCode, const char* layerPrefix, const char* message, void* data) {
	FILE* output;
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		output = stderr;
	else
		output = stdout;
	fprintf(output, "[VK2D] Callback: %i/%i/%" PRIu64 "/%" PRIu64 "/%i (%s) %s\n", flags, objectType, sourceObject, (uint64_t)location, messageCode, layerPrefix, message);
	fflush(output);
#ifdef VK2D_ERROR_FILE
	FILE *file = fopen(VK2D_ERROR_FILE, "a");
	fprintf(file, "[VK2D] Callback: %i/%i/%" PRIu64 "/%" PRIu64 "/%i (%s) %s\n", flags, objectType, sourceObject, (uint64_t)location, messageCode, layerPrefix, message);
	fclose(file);
#endif // VK2D_ERROR_FILE
#ifdef VK2D_QUIT_ON_ERROR
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		abort();
#endif //VK2D_QUIT_ON_ERROR
	return false;
}