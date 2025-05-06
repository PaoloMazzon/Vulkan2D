/// \file Logger.c
/// \author cmburn

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include "VK2D/Logger.h"
#include "VK2D/Opaque.h"
#include "Validation.h"

typedef struct VK2DDefaultLogger {
	FILE *errorOutput;
	FILE *standardOutput;
} VK2DDefaultLogger;

static SDL_Mutex *gLoggerMutex = NULL;
static VK2DDefaultLogger gDefaultLogger;
static bool gInitialized = false;
static void defaultLog(void *ptr, VK2DLogSeverity severity, const char *msg);

static struct VK2DLogger_t gLogger = {
	.log = defaultLog,
	.destroy = NULL,
	.context = &gDefaultLogger,
};

#define MAX_SEVERITY_LABEL_LENGTH (sizeof("unknown") - 1)

static bool
usingDefaultLogger()
{
	return gLogger.context == &gDefaultLogger;
}

static const char *
severityLabel(const VK2DLogSeverity severity, size_t *length)
{
	switch (severity) {
	case VK2D_LOG_SEVERITY_DEBUG:
		*length = sizeof("debug") - 1;
		return "debug";
	case VK2D_LOG_SEVERITY_INFO:
		*length = sizeof("info") - 1;
		return "info";
	case VK2D_LOG_SEVERITY_WARN:
		*length = sizeof("warn") - 1;
		return "warn";
	case VK2D_LOG_SEVERITY_ERROR:
		*length = sizeof("error") - 1;
		return "error";
	case VK2D_LOG_SEVERITY_FATAL:
		*length = sizeof("fatal") - 1;
		return "fatal";
	case VK2D_LOG_SEVERITY_UNKNOWN:
	default: // fallthrough
		*length = sizeof("unknown") - 1;
		return "unknown";
	}
}



static FILE *
defaultLogOutput(const VK2DDefaultLogger *log, const VK2DLogSeverity severity)
{
	switch (severity) {
	case VK2D_LOG_SEVERITY_WARN:
	case VK2D_LOG_SEVERITY_ERROR: // fallthrough
	case VK2D_LOG_SEVERITY_FATAL: // fallthrough
		return log->errorOutput;
	default:
		return log->standardOutput;
	}
}

#define MAX_TIME_STRING_SIZE 26

static void
writeTimeString(char *buf)
{
	time_t t = time(NULL);
	struct tm tm;
	errno_t err;
#ifdef _UCRT
	err = localtime_s(&tm, &t);
#else
	struct tm *t_ptr = localtime_s(&t, &buf);
	err = errno();
#endif
	assert(err == 0);
	err = asctime_s(buf, MAX_TIME_STRING_SIZE, &tm);
	assert(err == 0);
}

static void
defaultLog(void *ptr, const VK2DLogSeverity severity, const char *msg)
{
	const VK2DDefaultLogger *log = (VK2DDefaultLogger *)ptr;
	assert(gLogger.context == &gDefaultLogger);
	FILE *out = defaultLogOutput(log, severity);
	size_t labelLength = 0;
	const char *label = severityLabel(severity, &labelLength);
	char padding[MAX_SEVERITY_LABEL_LENGTH + 1];
	char timeString[MAX_TIME_STRING_SIZE];
	size_t paddingLength = MAX_SEVERITY_LABEL_LENGTH - labelLength;
	for (int i = 0; i < paddingLength; i++) {
		padding[i] = ' ';
	}
	padding[paddingLength] = '\0';
	writeTimeString(timeString);
	// asctime adds an extra \n at the end
	timeString[MAX_TIME_STRING_SIZE - 2] = '\0';
	fprintf(out, "[%s]%s[%s] %s\n", timeString, padding, label, msg);
	fflush(out);
	if (severity == VK2D_LOG_SEVERITY_FATAL)
		abort();
}

void
vk2dSetLogger(void *context, const VK2DLoggerLogFn log,
    const VK2DLoggerDestroyFn destroy)
{
	SDL_LockMutex(gLoggerMutex);
	gLogger.context = context;
	gLogger.log = log;
	gLogger.destroy = destroy;
	SDL_UnlockMutex(gLoggerMutex);
}

void
vk2dLoggerLogf(const VK2DLogSeverity severity, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vk2dLoggerLogv(severity, fmt, args);
	va_end(args);
}

void
vk2dLoggerLogv(const VK2DLogSeverity severity, const char *fmt, va_list ap)
{
	// avoid allocation if possible
	char msgBuf[128];
	size_t len = vsnprintf(NULL, 0, fmt, ap);
	size_t bufLen = sizeof(msgBuf) / sizeof(char);
	char *buf = msgBuf;
	if (len + 1 > bufLen) {
		bufLen = len + 1;
		buf = calloc(bufLen, sizeof(char));
	}
	if (buf == NULL) {
		vk2dRaise(VK2D_STATUS_OUT_OF_RAM,
			"Failed to allocate memory for error message");
		return;
	}
	vsnprintf(buf, len + 1, fmt, ap);
	vk2dLoggerLog(severity, buf);
	if (buf != msgBuf) free(buf);
}

void
vk2dLoggerLog(const VK2DLogSeverity severity, const char *msg)
{
	SDL_LockMutex(gLoggerMutex);
	gLogger.log(gLogger.context, severity, msg);
	SDL_UnlockMutex(gLoggerMutex);
}
void
vk2dLoggerDestroy()
{
	SDL_LockMutex(gLoggerMutex);
	if (gLogger.destroy != NULL) gLogger.destroy(gLogger.context);
	SDL_UnlockMutex(gLoggerMutex);
	SDL_DestroyMutex(gLoggerMutex);
}

void
vk2dLoggerInit()
{
	if (gInitialized) return;
	gLoggerMutex = SDL_CreateMutex();
	gDefaultLogger.errorOutput = stderr;
	gDefaultLogger.standardOutput = stdout;
	gInitialized = true;
}

void
_vk2dDefaultLoggerSetStandardOutput(FILE *out)
{
	SDL_LockMutex(gLoggerMutex);
	gDefaultLogger.standardOutput = out;
	SDL_UnlockMutex(gLoggerMutex);
}

void
_vk2dDefaultLoggerSetErrorOutput(FILE *out)
{
	SDL_LockMutex(gLoggerMutex);
	gDefaultLogger.errorOutput = out;
	SDL_UnlockMutex(gLoggerMutex);
}
