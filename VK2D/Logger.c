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
	FILE **errorOutput;
	size_t errorOutputCount;
	FILE **standardOutput;
	size_t standardOutputCount;
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

// We use arrays in some places for severity data, this should be called on any
// user-supplied severity level to make sure we don't have an out of bounds
// issue.
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

static FILE **
defaultLogOutput(const VK2DDefaultLogger *log, const VK2DLogSeverity severity,
    size_t *count)
{
	switch (severity) {
	case VK2D_LOG_SEVERITY_WARN:
	case VK2D_LOG_SEVERITY_ERROR: // fallthrough
	case VK2D_LOG_SEVERITY_FATAL: // fallthrough
		*count = log->errorOutputCount;
		return log->errorOutput;
	default: *count = log->standardOutputCount; return log->standardOutput;
	}
}

static VK2DLogSeverity
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
	if (gLogger->severityFn == NULL) return true;
	const VK2DLogSeverity current = gLogger->severityFn(gLogger->context);
	SDL_UnlockMutex(gLoggerMutex);
	return severity >= current;
}

static void
writeTimeString(char *buf)
{
	static const char *WEEK_DAYS[]
	    = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char *MONTHS[] = { "Jan", "Feb", "Mar", "Apr", "May",
		"Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	SDL_Time time;
	SDL_DateTime dt;

	if (!SDL_GetCurrentTime(&time)
	    || !SDL_TimeToDateTime(time, &dt, true)) {
		memset(buf, '-', MAX_TIME_STRING_SIZE);
		return;
	}

	snprintf(buf, MAX_TIME_STRING_SIZE, "%s %s %i %02d:%02d:%02d %i",
	    WEEK_DAYS[dt.day_of_week], MONTHS[dt.month - 1], dt.day, dt.hour,
	    dt.minute, dt.second, dt.year);
}

static void
destroyLogger(const bool lock)
{
	if (lock) SDL_LockMutex(gLoggerMutex);
	if (gLogger != NULL) {
		if (gLogger->destroy != NULL)
			gLogger->destroy(gLogger->context);
		gLogger = NULL;
	}
	if (lock) SDL_UnlockMutex(gLoggerMutex);
}

static void
defaultLog(void *ptr, VK2DLogSeverity severity, const char *msg)
{
	const VK2DDefaultLogger *log = (VK2DDefaultLogger *)ptr;
	assert(usingDefaultLogger());
	COERCE_SEVERITY(severity);
	size_t count = 0;
	FILE **out = defaultLogOutput(log, severity, &count);
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
	for (size_t i = 0; i < count; i++) {
		FILE *outFile = out[i];
		if (outFile == NULL) continue;
		fprintf(outFile, "[%s] [%s]%s %s\n", timeString, label, padding,
		    msg);
		fflush(outFile);
	}
	if (severity == VK2D_LOG_SEVERITY_FATAL)
	    abort();
}

void
vk2dSetLogger(VK2DLogger *logger)
{
	SDL_LockMutex(gLoggerMutex);
	destroyLogger(false);
	gLogger = logger;
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
vk2dLoggerLogv(VK2DLogSeverity severity, const char *fmt, va_list ap)
{
	COERCE_SEVERITY(severity);
	// avoid allocation if possible
	char msgBuf[128];
	const size_t len = vsnprintf(NULL, 0, fmt, ap);
	size_t bufLen = sizeof(msgBuf) / sizeof(char);
	char *buf = msgBuf;
	if (len + 1 > bufLen) {
		bufLen = len + 1;
		buf = calloc(bufLen, sizeof(char));
		if (buf == NULL) {
			vk2dRaise(VK2D_STATUS_OUT_OF_RAM,
			    "Failed to allocate memory for error message");
			return;
		}
	}
	vsnprintf(buf, len + 1, fmt, ap);
	vk2dLoggerLog(severity, buf);
	if (buf != msgBuf) free(buf);
}

void
vk2dLoggerLog(const VK2DLogSeverity severity, const char *msg)
{
	gLogger->log(gLogger->context, severity, msg);
	assert(severity != VK2D_LOG_SEVERITY_FATAL);
#ifdef NDEBUG
	if (severity == VK2D_LOG_SEVERITY_FATAL) abort();
#endif
}

static void
closeOutput(FILE **outputs, size_t count)
{
	if (outputs == NULL) return;
	for (size_t i = 0; i < count; i++) {
		FILE *fh = outputs[i];
		if (fh != stderr && fh != stdout) fclose(fh);
	}
	free(outputs);
}

void
vk2dLoggerDestroy()
{
	destroyLogger(true);
	SDL_DestroyMutex(gLoggerMutex);
	SDL_LockMutex(gDefaultLogger.mutex);
	closeOutput(gDefaultLogger.errorOutput,
	    gDefaultLogger.errorOutputCount);
	closeOutput(gDefaultLogger.standardOutput,
	    gDefaultLogger.standardOutputCount);
	SDL_UnlockMutex(gDefaultLogger.mutex);
	SDL_DestroyMutex(gDefaultLogger.mutex);
}

void
vk2dLoggerInit()
{
	if (gInitialized) return;
	gDefaultLogger.errorOutput = malloc(sizeof(FILE *));
	gDefaultLogger.errorOutputCount = 1;
	gDefaultLogger.errorOutput[0] = stderr;
	gDefaultLogger.standardOutput = malloc(sizeof(FILE *));
	gDefaultLogger.standardOutputCount = 1;
	gDefaultLogger.standardOutput[0] = stdout;
	gDefaultLogger.mutex = SDL_CreateMutex();
	gLoggerMutex = SDL_CreateMutex();
	gLogger = (VK2DLogger *)&gDefaultLogger;
	gInitialized = true;
}

void
addOutput(FILE ***output, size_t *count, SDL_Mutex *mutex, FILE *value)
{
	SDL_LockMutex(mutex);
	if (*output == NULL) {
		*output = malloc(sizeof(FILE *));
		(*output)[0] = value;
		*count = 1;
	} else {
		FILE **newOutput
		    = realloc(*output, sizeof(FILE *) * (*count + 1));
		if (newOutput == NULL)
			vk2dLogFatal("Failed to allocate memory for outputs");
		newOutput[*count] = value;
		*output = newOutput;
		(*count)++;
	}
	SDL_UnlockMutex(mutex);
}

void
vk2dDefaultLoggerAddStandardOutput(FILE *out)
{
	addOutput(&gDefaultLogger.standardOutput,
	    &gDefaultLogger.standardOutputCount, gDefaultLogger.mutex, out);
}

void
vk2dDefaultLoggerAddErrorOutput(FILE *out)
{
	addOutput(&gDefaultLogger.errorOutput, &gDefaultLogger.errorOutputCount,
	    gDefaultLogger.mutex, out);
}

void
vk2dDefaultLoggerSetSeverity(const VK2DLogSeverity severity)
{
	SDL_LockMutex(gDefaultLogger.mutex);
	gDefaultLogger.severity = severity;
	SDL_UnlockMutex(gDefaultLogger.mutex);
}

#define LOG_WRAP_FN(NAME, SEVERITY)                                            \
	void vk2dLog##NAME##v(const char *fmt, va_list ap)                     \
	{                                                                      \
		vk2dLoggerLogv(VK2D_LOG_SEVERITY_##SEVERITY, fmt, ap);         \
	}                                                                      \
                                                                               \
	void vk2dLog##NAME(const char *fmt, ...)                               \
	{                                                                      \
		va_list ap;                                                    \
		va_start(ap, fmt);                                             \
		vk2dLoggerLogv(VK2D_LOG_SEVERITY_##SEVERITY, fmt, ap);         \
		va_end(ap);                                                    \
	}

LOG_WRAP_FN(Debug, DEBUG)
LOG_WRAP_FN(Info, INFO)
LOG_WRAP_FN(Warn, WARN)
LOG_WRAP_FN(Error, ERROR)

void
vk2dLogFatalv(const char *fmt, va_list ap)
{
	vk2dLoggerLogv(VK2D_LOG_SEVERITY_FATAL, fmt, ap);
	abort();
}

void
vk2dLogFatal(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vk2dLoggerLogv(VK2D_LOG_SEVERITY_FATAL, fmt, ap);
	va_end(ap);
	abort();
}
