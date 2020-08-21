/// \file DescriptorControl.h
/// \author Paolo Mazzon
/// \brief Makes dynamic descriptor control simpler
#pragma once
#include "VK2D/Structs.h"

/// \brief Abstraction for descriptor pools and sets so you can dynamically use them
struct VK2DDescriptorControl {
	VkDescriptorPool *pools; ///< List of pools
	uint32_t *poolsUsage;    ///< How many descriptor sets currently in use by a given pool

	// pools will always have poolListSize elements, but only elements up to poolsInUse will be
	// valid pools (in an effort to avoid constantly reallocating memory)
	uint32_t poolsInUse;   ///< Number of actively in use pools in pools
	uint32_t poolListSize; ///< Total length of pools array
};