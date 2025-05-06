/// \file Initializers.h
/// \author cmburn
/// \brief Logging callbacks

#pragma once

#include "VK2D/Structs.h"

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
void vk2dDefaultLoggerSetStandardOutput(FILE *out);

/// \brief Sets the error output for the default global logger
void vk2dDefaultLoggerSetErrorOutput(FILE *out);

/// \brief Sets the minimum severity level for logging in the default logger.
void vk2dDefaultLoggerSetSeverity(VK2DLogSeverity severity);
