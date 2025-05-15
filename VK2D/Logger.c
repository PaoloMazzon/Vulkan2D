/// \file Logger.c
/// \author cmburn

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "VK2D/Logger.h"
#include "VK2D/Opaque.h"
#include "VK2D/Validation.h"

#define MAX_SEVERITY_LABEL_LENGTH (sizeof("unknown") - 1)
#define MAX_TIME_STRING_SIZE 26

#define COERCE_SEVERITY(SEVERITY)                                              \
	do {                                                                   \
		(SEVERITY) = coerceSeverity(SEVERITY);                         \
		if (!shouldLog(SEVERITY)) return;                              \
	} while (0)

typedef struct VK2DDefaultLogger {
	VK2DLogger log;
	SDL_Mutex *mutex;
	FILE *errorOutput;
	FILE *standardOutput;
	VK2DLogSeverity severity;
} VK2DDefaultLogger;

static VK2DLogger *gLogger;
static SDL_Mutex *gLoggerMutex;
static bool gInitialized = false;
static void defaultLog(void *ptr, VK2DLogSeverity severity, const char *msg);
static VK2DLogSeverity defaultSeverity(void *);

static VK2DDefaultLogger gDefaultLogger = {
	.log = {
		.log = defaultLog,
		.destroy = NULL,
		.severityFn = defaultSeverity,
		.context = &gDefaultLogger,
	},
	.mutex = NULL,
	.errorOutput = NULL,
	.standardOutput = NULL,
	.severity = VK2D_LOG_SEVERITY_INFO,
};

static bool
usingDefaultLogger()
{
	return gLogger == (VK2DLogger *)&gDefaultLogger;
}

static VK2DLogSeverity
coerceSeverity(const VK2DLogSeverity severity)
{
	// ensure we've been passed a valid severity level
	switch (severity) {
	case VK2D_LOG_SEVERITY_DEBUG: return VK2D_LOG_SEVERITY_DEBUG;
	case VK2D_LOG_SEVERITY_INFO: return VK2D_LOG_SEVERITY_INFO;
	case VK2D_LOG_SEVERITY_WARN: return VK2D_LOG_SEVERITY_WARN;
	case VK2D_LOG_SEVERITY_ERROR: return VK2D_LOG_SEVERITY_ERROR;
	case VK2D_LOG_SEVERITY_FATAL: return VK2D_LOG_SEVERITY_FATAL;
	case VK2D_LOG_SEVERITY_UNKNOWN:
	default: // fallthrough
		return VK2D_LOG_SEVERITY_UNKNOWN;
	}
}

static const char *
severityLabel(const VK2DLogSeverity severity, size_t *length)
{
	static const char *LABELS[] = {
		[VK2D_LOG_SEVERITY_DEBUG] = "debug",
		[VK2D_LOG_SEVERITY_INFO] = "info",
		[VK2D_LOG_SEVERITY_WARN] = "warn",
		[VK2D_LOG_SEVERITY_ERROR] = "error",
		[VK2D_LOG_SEVERITY_FATAL] = "fatal",
		[VK2D_LOG_SEVERITY_UNKNOWN] = "unknown",
	};
	static const size_t LABEL_SIZE[] = {
		[VK2D_LOG_SEVERITY_DEBUG] = sizeof("debug") - 1,
		[VK2D_LOG_SEVERITY_INFO] = sizeof("info") - 1,
		[VK2D_LOG_SEVERITY_WARN] = sizeof("warn") - 1,
		[VK2D_LOG_SEVERITY_ERROR] = sizeof("error") - 1,
		[VK2D_LOG_SEVERITY_FATAL] = sizeof("fatal") - 1,
		[VK2D_LOG_SEVERITY_UNKNOWN] = sizeof("unknown") - 1,
	};
	*length = LABEL_SIZE[severity];
	return LABELS[severity];
}

static FILE *
defaultLogOutput(const VK2DDefaultLogger *log, const VK2DLogSeverity severity)
{
	switch (severity) {
	case VK2D_LOG_SEVERITY_WARN:
	case VK2D_LOG_SEVERITY_ERROR: // fallthrough
	case VK2D_LOG_SEVERITY_FATAL: // fallthrough
		return log->errorOutput;
	default: return log->standardOutput;
	}
}

VK2DLogSeverity
defaultSeverity(void *ptr)
{
	const VK2DDefaultLogger *log = ptr;
	SDL_LockMutex(log->mutex);
	const VK2DLogSeverity severity = log->severity;
	SDL_UnlockMutex(log->mutex);
	return severity;
}

static bool
shouldLog(const VK2DLogSeverity severity)
{
	if (severity == VK2D_LOG_SEVERITY_UNKNOWN
	    || severity == VK2D_LOG_SEVERITY_FATAL)
		return true;
	SDL_LockMutex(gLoggerMutex);
	const VK2DLogSeverity current = gLogger->severityFn(gLogger->context);
	SDL_UnlockMutex(gLoggerMutex);
	return severity >= current;
}

const char *WEEK_DAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static void
writeTimeString(char *buf)
{
    SDL_Time time;
    SDL_DateTime dt;
    assert(SDL_GetCurrentTime(&time));
    assert(SDL_TimeToDateTime(time, &dt, true));
    snprintf(buf, MAX_TIME_STRING_SIZE, "%s %s %i %02d:%02d:%02d %i", WEEK_DAYS[dt.day_of_week], MONTHS[dt.month - 1], dt.day, dt.hour, dt.minute, dt.second, dt.year);
}

static void
destroyLogger(const bool lock)
{
	if (lock) SDL_LockMutex(gLoggerMutex);
	if (gLogger == NULL) return;
	if (gLogger->destroy != NULL) gLogger->destroy(gLogger->context);
	gLogger = NULL;
	if (lock) SDL_UnlockMutex(gLoggerMutex);
}

static void
defaultLog(void *ptr, VK2DLogSeverity severity, const char *msg)
{
	const VK2DDefaultLogger *log = (VK2DDefaultLogger *)ptr;
	assert(usingDefaultLogger());
	COERCE_SEVERITY(severity);
	FILE *out = defaultLogOutput(log, severity);
	size_t labelLength = 0;
	const char *label = severityLabel(severity, &labelLength);
	char padding[MAX_SEVERITY_LABEL_LENGTH + 1];
	char timeString[MAX_TIME_STRING_SIZE];
	const size_t paddingLength = MAX_SEVERITY_LABEL_LENGTH - labelLength;
	for (int i = 0; i < paddingLength; i++) { padding[i] = ' '; }
	padding[paddingLength] = '\0';
	writeTimeString(timeString);
	// asctime() adds an extra \n at the end
	timeString[MAX_TIME_STRING_SIZE - 2] = '\0';
	fprintf(out, "[%s]%s[%s] %s\n", timeString, padding, label, msg);
	fflush(out);
	if (severity == VK2D_LOG_SEVERITY_FATAL) abort();
}

void
vk2dSetLogger(VK2DLogger *log)
{
	SDL_LockMutex(gLoggerMutex);
	destroyLogger(false);
	gLogger = log;
	SDL_UnlockMutex(gLoggerMutex);
}

void
vk2dLoggerLogf(VK2DLogSeverity severity, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vk2dLoggerLogv(severity, fmt, args);
	va_end(args);
}

void
vk2dLoggerLogv(VK2DLogSeverity severity, const char *fmt, va_list ap)
{
	COERCE_SEVERITY(severity);
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
	gLogger->log(gLogger->context, severity, msg);
}
void
vk2dLoggerDestroy()
{
	destroyLogger(true);
	SDL_DestroyMutex(gLoggerMutex);
	SDL_DestroyMutex(gDefaultLogger.mutex);
}

void
vk2dLoggerInit()
{
	if (gInitialized) return;
	gDefaultLogger.errorOutput = stderr;
	gDefaultLogger.standardOutput = stdout;
	gDefaultLogger.mutex = SDL_CreateMutex();
	gLoggerMutex = SDL_CreateMutex();
	gLogger = (VK2DLogger *)&gDefaultLogger;
	gInitialized = true;
}

void
vk2dDefaultLoggerSetStandardOutput(FILE *out)
{
	SDL_LockMutex(gDefaultLogger.mutex);
	gDefaultLogger.standardOutput = out;
	SDL_UnlockMutex(gDefaultLogger.mutex);
}

void
vk2dDefaultLoggerSetErrorOutput(FILE *out)
{
	SDL_LockMutex(gDefaultLogger.mutex);
	gDefaultLogger.errorOutput = out;
	SDL_UnlockMutex(gDefaultLogger.mutex);
}
void
vk2dDefaultLoggerSetSeverity(const VK2DLogSeverity severity)
{
	SDL_LockMutex(gDefaultLogger.mutex);
	gDefaultLogger.severity = severity;
	SDL_UnlockMutex(gDefaultLogger.mutex);
}
