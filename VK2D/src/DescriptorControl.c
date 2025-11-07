/// \file DescriptorControl.c
/// \author Paolo Mazzon
#include "VK2D/DescriptorControl.h"
#include "VK2D/Texture.h"
#include "VK2D/Image.h"
#include "VK2D/Buffer.h"
#include "VK2D/Constants.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/Opaque.h"
#include <malloc.h>

// Places another descriptor pool at the end of a given desc con's list, extending the list if need be
static void _vk2dDescConAppendList(VK2DDescCon descCon) {
    if (vk2dStatusFatal())
        return;
	if (descCon->poolListSize == descCon->poolsInUse) {
		VkDescriptorPool *newList = realloc(descCon->pools, sizeof(VkDescriptorPool) * (descCon->poolListSize + VK2D_DEFAULT_ARRAY_EXTENSION));
		if (newList != NULL) {
			descCon->poolListSize += VK2D_DEFAULT_ARRAY_EXTENSION;
			descCon->pools = newList;
		} else {
		    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to append descriptor control list.");
		    return;
		}
	}

	VkDescriptorPoolSize sizes[2];
	uint32_t i = 0;
	if (descCon->buffer != VK2D_NO_LOCATION) {
		sizes[i].descriptorCount = VK2D_DEFAULT_DESCRIPTOR_POOL_ALLOCATION;
		sizes[i].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		i++;
	}
	if (descCon->sampler != VK2D_NO_LOCATION) {
		sizes[i].descriptorCount = VK2D_DEFAULT_DESCRIPTOR_POOL_ALLOCATION;
		sizes[i].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		i++;
	}
	if (descCon->storageBuffer != VK2D_NO_LOCATION) {
		sizes[i].descriptorCount = VK2D_DEFAULT_DESCRIPTOR_POOL_ALLOCATION;
		sizes[i].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		i++;
	}
	VkDescriptorPoolCreateInfo createInfo = vk2dInitDescriptorPoolCreateInfo(sizes, i, VK2D_DEFAULT_DESCRIPTOR_POOL_ALLOCATION);
	VkResult result = vkCreateDescriptorPool(descCon->dev->dev, &createInfo, VK_NULL_HANDLE, &descCon->pools[descCon->poolsInUse]);
	if (result != VK_SUCCESS) {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create descriptor pool, Vulkan error %i.", result);
	}
	descCon->poolsInUse++;
}

// Gets the first available descriptor set from a descriptor controller (allocating a new pool if need be)
VkDescriptorSet _vk2dDescConGetAvailableSet(VK2DDescCon descCon) {
    if (vk2dStatusFatal())
        return VK_NULL_HANDLE;

	VkDescriptorSet set = VK_NULL_HANDLE;
	uint32_t i = 0;
	VkResult res;
	VkDescriptorSetAllocateInfo allocInfo = vk2dInitDescriptorSetAllocateInfo(VK_NULL_HANDLE, 1, &descCon->layout);

	while (set == VK_NULL_HANDLE) {
		allocInfo.descriptorPool = descCon->pools[i];
		res = vkAllocateDescriptorSets(descCon->dev->dev, &allocInfo, &set);
		if (res == VK_ERROR_OUT_OF_POOL_MEMORY) {
			set = VK_NULL_HANDLE;
		} else if (res != VK_SUCCESS) {
            vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create descriptor set, Vulkan error %i.", res);
			break;
		}
		i++;
		if (i == descCon->poolsInUse && set == VK_NULL_HANDLE)
			_vk2dDescConAppendList(descCon);
	}

	return set;
}

VK2DDescCon vk2dDescConCreate(VK2DLogicalDevice dev, VkDescriptorSetLayout layout, uint32_t buffer, uint32_t sampler, uint32_t storageBuffer) {
    if (vk2dStatusFatal())
        return NULL;

	VK2DDescCon out = calloc(1, sizeof(struct VK2DDescCon_t));

	if (out != NULL) {
		out->layout = layout;
		out->buffer = buffer;
		out->storageBuffer = storageBuffer;
		out->sampler = sampler;
		out->dev = dev;
		out->pools = NULL;
		out->poolListSize = 0;
		out->poolsInUse = 0;
		_vk2dDescConAppendList(out);
	} else {
	    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate descriptor controller.");
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

VkDescriptorSet vk2dDescConGetBufferSet(VK2DDescCon descCon, VK2DBuffer buffer) {
    if (vk2dStatusFatal())
        return VK_NULL_HANDLE;

	VkDescriptorSet set = _vk2dDescConGetAvailableSet(descCon);
	VkDescriptorBufferInfo bufferInfo = {0};
	bufferInfo.buffer = buffer->buf;
	bufferInfo.offset = 0;
	bufferInfo.range = buffer->size;
	VkWriteDescriptorSet write = vk2dInitWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descCon->buffer, set,
																&bufferInfo, 1, VK_NULL_HANDLE);
	vkUpdateDescriptorSets(descCon->dev->dev, 1, &write, 0, VK_NULL_HANDLE);
	return set;
}

VkDescriptorSet vk2dDescConGetSet(VK2DDescCon descCon) {
    if (vk2dStatusFatal())
        return VK_NULL_HANDLE;

	return _vk2dDescConGetAvailableSet(descCon);
}

VkDescriptorSet vk2dDescConGetSamplerSet(VK2DDescCon descCon, VK2DTexture tex) {
    if (vk2dStatusFatal())
        return VK_NULL_HANDLE;

	VkDescriptorSet set = _vk2dDescConGetAvailableSet(descCon);
	VkDescriptorImageInfo imageInfo = {0};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = tex->img->view;
	VkWriteDescriptorSet write = vk2dInitWriteDescriptorSet(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
																descCon->sampler, set, VK_NULL_HANDLE, 1, &imageInfo);
	vkUpdateDescriptorSets(descCon->dev->dev, 1, &write, 0, VK_NULL_HANDLE);
	return set;
}

VkDescriptorSet vk2dDescConGetSamplerBufferSet(VK2DDescCon descCon, VK2DTexture tex, VK2DBuffer buffer) {
    if (vk2dStatusFatal())
        return VK_NULL_HANDLE;

	VkDescriptorSet set = _vk2dDescConGetAvailableSet(descCon);
	VkWriteDescriptorSet write[2];
	VkDescriptorImageInfo imageInfo = {0};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = tex->img->view;
	write[1] = vk2dInitWriteDescriptorSet(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descCon->sampler, set,
											  VK_NULL_HANDLE, 1, &imageInfo);
	VkDescriptorBufferInfo bufferInfo = {0};
	bufferInfo.buffer = buffer->buf;
	bufferInfo.offset = 0;
	bufferInfo.range = buffer->size;
	write[0] = vk2dInitWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descCon->buffer, set, &bufferInfo, 1,
											  VK_NULL_HANDLE);
	vkUpdateDescriptorSets(descCon->dev->dev, 2, write, 0, VK_NULL_HANDLE);
	return set;
}

void vk2dDescConReset(VK2DDescCon descCon) {
    if (vk2dStatusFatal())
        return;

	uint32_t i;
	for (i = 0; i < descCon->poolsInUse; i++) {
		vkResetDescriptorPool(descCon->dev->dev, descCon->pools[i], 0);
	}
}
