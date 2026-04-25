/// \file Compat.h
/// \author Baptiste Guerin
/// \brief Backward-compatible aliases for legacy math type names and for users
/// who don't use cglm.

#pragma once

#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef VK2DVec2 vec2;
typedef VK2DVec3 vec3;
typedef VK2DVec4 vec4;
typedef VK2DMat4 mat4;

#ifdef __cplusplus
};
#endif
