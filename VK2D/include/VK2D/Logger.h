/// \file Initializers.h
/// \author cmburn
/// \brief Logging callbacks

#pragma once

#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Initializes the global values for logging
void vk2dLoggerInit();

/// \brief Frees allocated logging resources
void vk2dLoggerDestroy();

/// \brief Supply user context and callbacks for logger
/// \note NULL may be passed to reset to the default logger.
/// \note The current logger will be destroyed if it has a destroy() callback.
void vk2dSetLogger(VK2DLogger *logger);

/// \brief Log a message with a printf() style string
void vk2dLoggerLogf(VK2DLogSeverity severity, const char *fmt, ...);

/// \brief Log a message with a vprintf() style string
void vk2dLoggerLogv(VK2DLogSeverity severity, const char *fmt, va_list ap);

/// \brief Log a message
void vk2dLoggerLog(VK2DLogSeverity severity, const char *msg);

/// \brief Sets the standard output for the default global logger
void vk2dDefaultLoggerSetStandardOutput(FILE *out);

/// \brief Sets the error output for the default global logger
void vk2dDefaultLoggerSetErrorOutput(FILE *out);

/// \brief Sets the minimum severity level for logging in the default logger.
void vk2dDefaultLoggerSetSeverity(VK2DLogSeverity severity);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_DEBUG severity
void vk2dLogDebug(const char *fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_DEBUG
///        severity
void vk2dLogDebugv(const char *fmt, va_list ap);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_INFO severity
void vk2dLogInfo(const char *fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_INFO severity
void vk2dLogInfov(const char *fmt, va_list ap);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_WARN severity
void vk2dLogWarn(const char *fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_WARN severity
void vk2dLogWarnv(const char *fmt, va_list ap);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_ERROR severity
void vk2dLogError(const char *fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_ERROR
/// severity
void vk2dLogErrorv(const char *fmt, va_list ap);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_FATAL severity
void vk2dLogFatal(const char *fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_FATAL
/// severity
void vk2dLogFatalv(const char *fmt, va_list ap);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_UNKNOWN
void vk2dLogUnknown(const char *fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_UNKNOWN
void vk2dLogUnknownv(const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif
