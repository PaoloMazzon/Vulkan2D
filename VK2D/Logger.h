/// \file Initializers.h
/// \author cmburn
/// \brief Logging callbacks

#pragma once

// need noreturn to suppress warnings

#if defined(__GNUC__) || defined(__clang__)
#define VK2D_NORETURN __attribute__((__noreturn__))
#elif defined(_MSC_VER)
#define VK2D_NORETURN __declspec(noreturn)
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define VK2D_NORETURN [[noreturn]]
#else
#define VK2D_NORETURN
#endif

#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif


/// \brief Initializes the global values for logging
void vk2dLoggerInit();

/// \brief Frees allocated logging resources
void vk2dLoggerDestroy();

/// \brief Supply user context and callbacks for logger
void vk2dSetLogger(VK2DLogger *logger);

/// \brief Log a message with a printf() style string
void vk2dLoggerLogf(VK2DLogSeverity severity, const char *fmt, ...);

/// \brief Log a message with a vprintf() style string
void vk2dLoggerLogv(VK2DLogSeverity severity, const char *fmt, va_list ap);

/// \brief Log a message
void vk2dLoggerLog(VK2DLogSeverity severity, const char *msg);

/// \brief Sets the standard output for the default global logger
void vk2dDefaultLoggerAddStandardOutput(FILE *out);

/// \brief Sets the error output for the default global logger
void vk2dDefaultLoggerAddErrorOutput(FILE *out);

/// \brief Sets the minimum severity level for logging in the default logger.
void vk2dDefaultLoggerSetSeverity(VK2DLogSeverity severity);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_DEBUG severity
void vk2dLogDebug(const char *fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_DEBUG
///        severity
void vk2dLogDebugv(const char *fmt, va_list ap);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_INFO severity
void vk2dLogInfo(const char* fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_INFO severity
void vk2dLogInfov(const char* fmt, va_list ap);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_WARN severity
void vk2dLogWarn(const char* fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_WARN severity
void vk2dLogWarnv(const char* fmt, va_list ap);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_ERROR severity
void vk2dLogError(const char* fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_ERROR severity
void vk2dLogErrorv(const char* fmt, va_list ap);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_FATAL severity
VK2D_NORETURN void vk2dLogFatal(const char* fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_FATAL severity
VK2D_NORETURN void vk2dLogFatalv(const char* fmt, va_list ap);

/// \brief Prints a printf() style message with VK2D_LOG_SEVERITY_UNKNOWN
void vk2dLogUnknown(const char* fmt, ...);

/// \brief Prints a vprintf() style message with VK2D_LOG_SEVERITY_UNKNOWN
void vk2dLogUnknownv(const char* fmt, va_list ap);

#ifdef __cplusplus
}
#endif
