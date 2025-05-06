/// \file Validation.c
/// \author Paolo Mazzon
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "VK2D/Validation.h"

#include "Logger.h"
#include "VK2D/Opaque.h"
#include "VK2D/Renderer.h"

static SDL_Mutex *gLogMutex = NULL;

// Global log buffer
static const int32_t gLogBufferSize = 4096;
static char gLogBuffer[4096] = "Vulkan2D is not initialized.";
static VK2DStatus gStatus;
static bool gResetLog;         // So the raise method knows to reset after the user gets the output
//static const char *gErrorFile; // Makes sure this file has access to the error log file immediately
static FILE *gErrorFile;
static bool gQuitOnError;      // Same as above

void vk2dValidationBegin(const char *errorFile, bool quitOnError) {
	vk2dLoggerInit();
	gLogMutex = SDL_CreateMutex();
	if (errorFile != NULL) {
		FILE *file = fopen(errorFile, "a");
		if (file != NULL) {
			vk2dDefaultLoggerSetErrorOutput(file);
			gErrorFile = file;
		}
	}
	gQuitOnError = quitOnError;
	gStatus = 0;
}

void vk2dValidationEnd() {
	vk2dLoggerLog(VK2D_LOG_SEVERITY_INFO, "------END------");
	vk2dLoggerDestroy();
	SDL_DestroyMutex(gLogMutex);
	if (gErrorFile != NULL) fclose(gErrorFile);
}

void vk2dValidationWriteHeader() {
	time_t t = time(NULL);
	vk2dLoggerLog(VK2D_LOG_SEVERITY_INFO, "------START------");
	vk2dLoggerLog(VK2D_LOG_SEVERITY_INFO, vk2dHostInformation());
}

void vk2dRaise(VK2DStatus result, const char* fmt, ...) {
    // Ignore double raises on "renderer not intialized"
    if (result == VK2D_STATUS_RENDERER_NOT_INITIALIZED && (gStatus & VK2D_STATUS_RENDERER_NOT_INITIALIZED) != 0)
        return;
    const VK2DLogSeverity severity = (gQuitOnError && vk2dStatusFatal()) ?
    	VK2D_LOG_SEVERITY_FATAL : VK2D_LOG_SEVERITY_ERROR;
    va_list ap;
    va_start(ap, fmt);
    vk2dLoggerLogv(severity, fmt, ap);
    va_end(ap);
}

VK2DStatus vk2dStatus() {
    gResetLog = true;
    return gStatus;
}

bool vk2dStatusFatal() {
    return (gStatus & ~(VK2D_STATUS_NONE | VK2D_STATUS_SDL_ERROR | VK2D_STATUS_FILE_NOT_FOUND | VK2D_STATUS_BAD_ASSET | VK2D_STATUS_TOO_MANY_CAMERAS)) !=  0;
}

void vk2dLog(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vk2dLoggerLogv(VK2D_LOG_SEVERITY_INFO, fmt, ap);
	va_end(ap);
}

VKAPI_ATTR VkBool32 VKAPI_CALL _vk2dDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t sourceObject, size_t location, int32_t messageCode, const char* layerPrefix, const char* message, void* data) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	char firstHalf[1000];
	const bool isError = flags & VK_DEBUG_REPORT_ERROR_BIT_EXT;
	const VK2DLogSeverity severity = isError
	    ? gRenderer->options.quitOnError
	    ? VK2D_LOG_SEVERITY_FATAL
	    : VK2D_LOG_SEVERITY_ERROR
	    : VK2D_LOG_SEVERITY_INFO;
	snprintf(firstHalf, 999, "%s", layerPrefix);
	vk2dLoggerLog(severity, firstHalf);
	vk2dLoggerLog(severity, message);
	if (gRenderer->options.quitOnError)
		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
			abort();
	return false;
}