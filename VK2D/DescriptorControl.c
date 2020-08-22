/// \file DescriptorControl.c
/// \author Paolo Mazzon
#include "VK2D/DescriptorControl.h"
#include "VK2D/Constants.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include <malloc.h>

VK2DDescCon vk2dDescConCreate(VkDescriptorSetLayout layout, bool buffer, bool sampler) {

}

void vk2dDescConFree(VK2DDescCon descCon) {

}

VkDescriptorSet *vk2dDescConGetBufferSet(VK2DDescCon descCon, void *buffer, uint32_t size, VK2DShaderStage stage) {

}

VkDescriptorSet *vk2dDescConGetSamplerSet(VK2DDescCon descCon, VK2DTexture tex, VK2DShaderStage stage) {

}

void vk2dDescConReset(VK2DDescCon descCon) {

}