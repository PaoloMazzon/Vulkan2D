/// \file DescriptorControl.c
/// \author Paolo Mazzon
#include "VK2D/DescriptorControl.h"
#include "VK2D/Constants.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include "VK2D/LogicalDevice.h"
#include <malloc.h>

// Places another descriptor pool at the end of a given desc con's list, extending the list if need be
static void _vk2dDescConAppendList(VK2DDescCon descCon) {
	if (descCon->poolListSize == descCon->poolsInUse) {
		VkDescriptorPool *newList = realloc(descCon->pools, sizeof(VkDescriptorPool) * (descCon->poolListSize + VK2D_DEFAULT_ARRAY_EXTENSION));
		if (vk2dPointerCheck(newList)) {
			descCon->poolListSize += VK2D_DEFAULT_ARRAY_EXTENSION;
			descCon->pools = newList;
		}
	}

	VkDescriptorPoolSize sizes[2];
	uint32_t sizeCount = (descCon->sampler != VK2D_NO_LOCATION ? 1 : 0) + (descCon->buffer != VK2D_NO_LOCATION ? 1 : 0);
	uint32_t i = 0;
	if (descCon->sampler != VK2D_NO_LOCATION) {
		sizes[i].descriptorCount = VK2D_DEFAULT_DESCRIPTOR_POOL_ALLOCATION;
		sizes[i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		i++;
	}
	if (descCon->buffer != VK2D_NO_LOCATION) {
		sizes[i].descriptorCount = VK2D_DEFAULT_DESCRIPTOR_POOL_ALLOCATION;
		sizes[i].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}
	VkDescriptorPoolCreateInfo createInfo = vk2dInitDescriptorPoolCreateInfo(sizes, sizeCount, VK2D_DEFAULT_DESCRIPTOR_POOL_ALLOCATION);
	vk2dErrorCheck(vkCreateDescriptorPool(descCon->dev->dev, &createInfo, VK_NULL_HANDLE, &descCon->pools[descCon->poolsInUse]));
	descCon->poolsInUse++;
}

VK2DDescCon vk2dDescConCreate(VK2DLogicalDevice dev, VkDescriptorSetLayout layout, uint32_t buffer, uint32_t sampler) {
	VK2DDescCon out = malloc(sizeof(struct VK2DDescCon));

	if (vk2dPointerCheck(out)) {
		out->layout = layout;
		out->buffer = buffer;
		out->sampler = sampler;
		out->dev = dev;
		out->pools = NULL;
		out->poolListSize = 0;
		out->poolsInUse = 0;
		_vk2dDescConAppendList(out);
	}

	return out;
}

void vk2dDescConFree(VK2DDescCon descCon) {
	if (descCon != NULL) {
		uint32_t i;
		for (i = 0; i < descCon->poolsInUse; i++)
			vkDestroyDescriptorPool(descCon->dev->dev, descCon->pools[i], VK_NULL_HANDLE);
		free(descCon->pools);
		free(descCon);
	}
}

VkDescriptorSet *vk2dDescConGetBufferSet(VK2DDescCon descCon, void *buffer, uint32_t size, VK2DShaderStage stage) {
	return VK_NULL_HANDLE; // TODO: This
}

VkDescriptorSet *vk2dDescConGetSamplerSet(VK2DDescCon descCon, VK2DTexture tex) {
	return VK_NULL_HANDLE; // TODO: This
}

VkDescriptorSet *vk2dDescConGetSamplerBufferSet(VK2DDescCon descCon, VK2DTexture tex, void *buffer, uint32_t size, VK2DShaderStage stage) {
	return VK_NULL_HANDLE; // TODO: This
}

void vk2dDescConReset(VK2DDescCon descCon) {
	uint32_t i;
	for (i = 0; i < descCon->poolsInUse; i++) {
		vkResetDescriptorPool(descCon->dev->dev, descCon->pools[i], 0);
	}
}