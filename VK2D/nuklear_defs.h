/// \brief Specific include order for Nuklear files
/// \author cmburm

#ifndef NUKLEAR_DEFS_H
#define NUKLEAR_DEFS_H

#include <assert.h>
#include <string.h>

#include <vulkan/vulkan.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT

#if __STDC_VERSION__ >= 202311L || defined(__cplusplus)
#define NK_STATIC_ASSERT(...) static_assert(__VA_ARGS__, #__VA_ARGS__)
#elif __STDC_VERSION__ >= 201112L
#define NK_STATIC_ASSERT(...) _Static_assert(__VA_ARGS__, #__VA_ARGS__)
#else
#ifndef __cplusplus
#error "C11 or greater is required"
#else
#define NK_ALIGNOF
#endif
#endif
#define NK_ASSERT(expr) assert(expr)
#define NK_MEMSET(ptr, val, size) memset(ptr, val, size)
#define NK_API extern
#define NK_INTERN static

#include "nuklear.h"
#include "nuklear_sdl_vulkan.h"

#endif /* NUKLEAR_DEFS_H */