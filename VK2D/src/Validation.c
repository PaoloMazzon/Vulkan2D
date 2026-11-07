/// \file Validation.c
/// \author Paolo Mazzon
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "VK2D/Validation.h"
#include "VK2D/Logger.h"
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
            vk2dDefaultLoggerSetErrorFile(file);
            gErrorFile = file;
        }
    }
    gQuitOnError = quitOnError;
    gStatus = 0;
}

void vk2dValidationEnd() {
    vk2dLoggerLog(VK2D_LOG_SEVERITY_INFO, "------END------");
    vk2dDefaultLoggerSetErrorFile(NULL);
    vk2dLoggerDestroy();
    SDL_DestroyMutex(gLogMutex);
    if (gErrorFile != NULL) fclose(gErrorFile);
    gErrorFile = NULL;
}

static int32_t stringLength(const char *str, int32_t size) {
    int32_t len = 0;
    for (int32_t i = 0; str[i] != 0 && i < size; i++) {
        len++;
    }
    return len;
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
    if (gResetLog) {
        gResetLog = false;
        gStatus = 0;
        gLogBuffer[0] = 0;
    }
    gStatus |= result;
    const VK2DLogSeverity severity = (gQuitOnError && vk2dStatusFatal()) ?
                                     VK2D_LOG_SEVERITY_FATAL : VK2D_LOG_SEVERITY_ERROR;
    va_list ap;
    int32_t start = stringLength(gLogBuffer, gLogBufferSize);
    int32_t length = gLogBufferSize - start;
    va_start(ap, fmt);
    vsnprintf(gLogBuffer + start, length, fmt, ap);
    va_end(ap);
    va_start(ap, fmt);
    vk2dLoggerLogv(severity, fmt, ap);
    va_end(ap);
}

VK2DStatus vk2dStatus() {
    gResetLog = true;
    return gStatus;
}

bool vk2dStatusFatal() {
    return (gStatus & ~(VK2D_STATUS_NONE | VK2D_STATUS_SDL_ERROR | VK2D_STATUS_FILE_NOT_FOUND | VK2D_STATUS_BAD_ASSET | VK2D_STATUS_TOO_MANY_CAMERAS | VK2D_STATUS_BEYOND_LIMIT)) !=  0;
}

const char *vk2dStatusMessage() {
    gResetLog = true;
    return gLogBuffer;
}

static const char* vk2dDebugReportFlagsToString(VkDebugReportFlagsEXT flags) {
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) return "ERROR";
    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) return "WARNING";
    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) return "PERF";
    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) return "INFO";
    if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) return "DEBUG";
    return "UNKNOWN";
}

static const char* vk2dDebugObjectTypeToString(VkDebugReportObjectTypeEXT type) {
    switch (type) {
        case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT: return "Instance";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT: return "Device";
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT: return "Buffer";
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT: return "Image";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT: return "Pipeline";
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT: return "CommandBuffer";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT: return "DescriptorSet";
        default: return "Unknown";
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL _vk2dDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t sourceObject, size_t location, int32_t messageCode, const char* layerPrefix, const char* message, void* data) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    VK2DLogSeverity severity = VK2D_LOG_SEVERITY_DEBUG;
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        severity = gRenderer->options.quitOnError ? VK2D_LOG_SEVERITY_FATAL : VK2D_LOG_SEVERITY_ERROR;
    else if (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
        severity = VK2D_LOG_SEVERITY_WARN;
    else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        severity = VK2D_LOG_SEVERITY_INFO;
    vk2dLoggerLogf(
            severity,
            "[VULKAN %s]\n"
            "  Layer     : %s\n"
            "  Object    : %s (0x%llx)\n"
            "  Location  : %zu\n"
            "  Code      : %d\n"
            "  Message   : %s",
            vk2dDebugReportFlagsToString(flags),
            layerPrefix,
            vk2dDebugObjectTypeToString(objectType),
            (unsigned long long)sourceObject,
            location,
            messageCode,
            message
    );
    if (gRenderer->options.quitOnError)
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
            abort();
    return false;
}