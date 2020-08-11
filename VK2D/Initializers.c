/// \file Initializers.c
/// \author Paolo Mazzon
#include "VK2D/Initializers.h"
#include "VK2D/Types.h"
#include <SDL2/SDL_vulkan.h>

#ifdef VK2D_ENABLE_DEBUG
static const char* EXTENSIONS[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static const char* LAYERS[] = {
		"VK_LAYER_LUNARG_standard_validation"
};
static const int LAYER_COUNT = 1;
static const int EXTENSION_COUNT = 1;
#else // VK2D_ENABLE_DEBUG
static const char* EXTENSIONS[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static const char* LAYERS[] = {

};
static const int LAYER_COUNT = 0;
static const int EXTENSION_COUNT = 1;
#endif // VK2D_ENABLE_DEBUG

VkApplicationInfo vk2dInitApplicationInfo(VK2DConfiguration *info) {
	VkApplicationInfo ret;
	ret.pApplicationName = info->applicationName;
	ret.pEngineName = info->engineName;
	ret.apiVersion = info->apiVersion;
	ret.applicationVersion = info->applicationVersion;
	ret.engineVersion = info->engineVersion;
	return ret;
}

VkInstanceCreateInfo vk2dInitInstanceCreateInfo(VkApplicationInfo *appInfo, const char **layers, int layerCount,
											  const char **extensions, int extensionCount) {
	VkInstanceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.pApplicationInfo = appInfo;
	info.enabledExtensionCount = (uint32_t)extensionCount;
	info.enabledLayerCount = (uint32_t)layerCount;
	info.ppEnabledExtensionNames = extensions;
	info.ppEnabledLayerNames = layers;

	return info;
}

VkDeviceQueueCreateInfo vk2dInitDeviceQueueCreateInfo(uint32_t queueFamilyIndex, float *priority) {
	VkDeviceQueueCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	info.queueFamilyIndex = queueFamilyIndex;
	info.queueCount = 1;
	info.pQueuePriorities = priority;
	return info;
}

VkDeviceCreateInfo vk2dInitDeviceCreateInfo(VkDeviceQueueCreateInfo *info, uint32_t size,
										  VkPhysicalDeviceFeatures *features) {
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = info;
	createInfo.queueCreateInfoCount = size;
	createInfo.pEnabledFeatures = features;
	createInfo.ppEnabledLayerNames = LAYERS;
	createInfo.ppEnabledExtensionNames = EXTENSIONS;
	createInfo.enabledLayerCount = LAYER_COUNT;
	createInfo.enabledExtensionCount = EXTENSION_COUNT;
	return createInfo;
}

VkCommandPoolCreateInfo vk2dInitCommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndex;
	poolInfo.flags = flags;
	return poolInfo;
}

VkCommandBufferAllocateInfo vk2dInitCommandBufferAllocateInfo(VkCommandPool pool, uint32_t count) {
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandBufferCount = count;
	allocateInfo.commandPool = pool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	return allocateInfo;
}

VkDebugReportCallbackCreateInfoEXT vk2dInitDebugReportCallbackCreateInfoEXT(PFN_vkDebugReportCallbackEXT callback) {
	VkDebugReportCallbackCreateInfoEXT createInfoEXT = {};
	createInfoEXT.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	createInfoEXT.pfnCallback = callback;
	createInfoEXT.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
	return createInfoEXT;
}

VkCommandBufferBeginInfo vk2dInitCommandBufferBeginInfo(VkCommandBufferUsageFlags flags) {
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = flags;
	return commandBufferBeginInfo;
}

VkSubmitInfo vk2dInitSubmitInfo(VkCommandBuffer *commandBufferVector, uint32_t commandBufferCount, VkSemaphore *signalSemaphoreVector, uint32_t signalSemaphoreCount, VkSemaphore *waitSemaphoreVector, uint32_t waitSemaphoreCount, VkPipelineStageFlags *waitFlag) {
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = commandBufferCount;
	submitInfo.pCommandBuffers = commandBufferVector;
	submitInfo.pSignalSemaphores = signalSemaphoreVector;
	submitInfo.signalSemaphoreCount = signalSemaphoreCount;
	submitInfo.pWaitSemaphores = waitSemaphoreVector;
	submitInfo.waitSemaphoreCount = waitSemaphoreCount;
	submitInfo.pWaitDstStageMask = waitFlag;
	return submitInfo;
}

VkFenceCreateInfo vk2dInitFenceCreateInfo(VkFenceCreateFlags flags) {
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = flags;
	return fenceCreateInfo;
}

VkSemaphoreCreateInfo vk2dInitSemaphoreCreateInfo(VkSemaphoreCreateFlags flags) {
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.flags = flags;
	return semaphoreCreateInfo;
}

VkSwapchainCreateInfoKHR vk2dInitSwapchainCreateInfoKHR(VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR capabilities, VkSurfaceFormatKHR format, uint32_t surfaceWidth, uint32_t surfaceHeight, VkPresentModeKHR presentMode, VkSwapchainKHR oldSwapchain, uint32_t minImageCount) {
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.imageColorSpace = format.colorSpace;
	swapchainCreateInfo.imageFormat = format.format;
	swapchainCreateInfo.imageExtent.width = surfaceWidth;
	swapchainCreateInfo.imageExtent.height = surfaceHeight;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.minImageCount = minImageCount;
	swapchainCreateInfo.oldSwapchain = oldSwapchain;
	return swapchainCreateInfo;
}

VkImageViewCreateInfo vk2dInitImageViewCreateInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectMask, uint32_t mipLevels) {
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	return imageViewCreateInfo;
}

VkImageCreateInfo vk2dInitImageCreateInfo(uint32_t imageWidth, uint32_t imageHeight, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels, VkSampleCountFlagBits samples) {
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = imageWidth;
	imageCreateInfo.extent.height = imageHeight;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = samples;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.format = format;
	return imageCreateInfo;
}

VkMemoryAllocateInfo vk2dInitMemoryAllocateInfo(uint32_t size, uint32_t memoryType) {
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = size;
	memoryAllocateInfo.memoryTypeIndex = memoryType;
	return memoryAllocateInfo;
}

VkRenderPassCreateInfo vk2dInitRenderPassCreateInfo(VkAttachmentDescription *attachments, uint32_t attachCount, VkSubpassDescription *subpasses, uint32_t subpassCount, VkSubpassDependency *deps, uint32_t depCount) {
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachCount;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.pSubpasses = subpasses;
	renderPassCreateInfo.subpassCount = subpassCount;
	renderPassCreateInfo.dependencyCount = depCount;
	renderPassCreateInfo.pDependencies = deps;
	return renderPassCreateInfo;
}

VkFramebufferCreateInfo vk2dInitFramebufferCreateInfo(VkRenderPass renderPass, uint32_t surfaceWidth, uint32_t surfaceHeight, VkImageView *attachments, uint32_t attachCount) {
	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.renderPass = renderPass;
	framebufferCreateInfo.width = surfaceWidth;
	framebufferCreateInfo.height = surfaceHeight;
	framebufferCreateInfo.layers = 1;
	framebufferCreateInfo.attachmentCount = attachCount;
	framebufferCreateInfo.pAttachments = attachments;
	return framebufferCreateInfo;
}

VkPresentInfoKHR vk2dInitPresentInfoKHR(VkSwapchainKHR *swapchains, uint32_t swapchainCount, uint32_t *images, VkResult *presentResult, VkSemaphore *waitSemaphores, uint32_t semaphoreCount) {
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = swapchainCount;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = images;
	presentInfo.pResults = presentResult;
	presentInfo.pWaitSemaphores = waitSemaphores;
	presentInfo.waitSemaphoreCount = semaphoreCount;
	return presentInfo;
}

VkRenderPassBeginInfo vk2dInitRenderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer framebuffer, VkRect2D rect2D, VkClearValue *clearValues, uint32_t clearCount) {
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = framebuffer;
	renderPassBeginInfo.renderArea = rect2D;
	renderPassBeginInfo.clearValueCount = clearCount;
	renderPassBeginInfo.pClearValues = clearValues;
	return renderPassBeginInfo;
}

VkShaderModuleCreateInfo vk2dInitShaderModuleCreateInfo(char *code, uint32_t codeSize) {
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pCode = (uint32_t*)code;
	shaderModuleCreateInfo.codeSize = codeSize;
	return shaderModuleCreateInfo;
}

VkPipelineShaderStageCreateInfo vk2dInitPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shader) {
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = stage;
	shaderStageCreateInfo.module = shader;
	shaderStageCreateInfo.pName = "main";
	return shaderStageCreateInfo;
}

VkPipelineVertexInputStateCreateInfo vk2dInitPipelineVertexInputStateCreateInfo(VkVertexInputBindingDescription *bindingDescriptions, uint32_t bindingDescriptionCount, VkVertexInputAttributeDescription *attributeDescriptions, uint32_t attributeDescriptionCount) {
	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions;
	pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = bindingDescriptions;
	pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = attributeDescriptionCount;
	pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = bindingDescriptionCount;
	return pipelineVertexInputStateCreateInfo;
}

VkPipelineInputAssemblyStateCreateInfo vk2dInitPipelineInputAssemblyStateCreateInfo() {
	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
	pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
	return pipelineInputAssemblyStateCreateInfo;
}

VkPipelineViewportStateCreateInfo vk2dInitPipelineViewportStateCreateInfo(VkViewport *viewport, VkRect2D *scissor) {
	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
	pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineViewportStateCreateInfo.pScissors = scissor;
	pipelineViewportStateCreateInfo.pViewports = viewport;
	pipelineViewportStateCreateInfo.scissorCount = 1;
	pipelineViewportStateCreateInfo.viewportCount = 1;
	return pipelineViewportStateCreateInfo;
}

VkPipelineRasterizationStateCreateInfo vk2dInitPipelineRasterizationStateCreateInfo() {
	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
	pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineRasterizationStateCreateInfo.lineWidth = 1;
	pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	return pipelineRasterizationStateCreateInfo;
}

VkPipelineMultisampleStateCreateInfo vk2dInitPipelineMultisampleStateCreateInfo(VkSampleCountFlagBits samples) {
	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
	pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	pipelineMultisampleStateCreateInfo.rasterizationSamples = samples;
	pipelineMultisampleStateCreateInfo.minSampleShading = 1;
	pipelineMultisampleStateCreateInfo.pSampleMask = VK_NULL_HANDLE;
	pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
	return pipelineMultisampleStateCreateInfo;
}

VkPipelineDepthStencilStateCreateInfo vk2dInitPipelineDepthStencilStateCreateInfo() {
	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {};
	pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	pipelineDepthStencilStateCreateInfo.minDepthBounds = 0;
	pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1;
	return pipelineDepthStencilStateCreateInfo;
}

VkPipelineColorBlendStateCreateInfo vk2dInitPipelineColorBlendStateCreateInfo(VkPipelineColorBlendAttachmentState *attachments, uint32_t attachmentCount) {
	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {};
	pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineColorBlendStateCreateInfo.logicOpEnable = VK_TRUE;
	pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
	pipelineColorBlendStateCreateInfo.pAttachments = attachments;
	return pipelineColorBlendStateCreateInfo;
}

VkPipelineDynamicStateCreateInfo vk2dInitPipelineDynamicStateCreateInfo(VkDynamicState *states, uint32_t stateCount) {
	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {};
	pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateCreateInfo.pDynamicStates = states;
	pipelineDynamicStateCreateInfo.dynamicStateCount = stateCount;
	return pipelineDynamicStateCreateInfo;
}

VkPipelineLayoutCreateInfo vk2dInitPipelineLayoutCreateInfo(VkDescriptorSetLayout *layouts, uint32_t layoutCount) {
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pSetLayouts = layouts;
	pipelineLayoutCreateInfo.setLayoutCount = layoutCount;
	return pipelineLayoutCreateInfo;
}

VkGraphicsPipelineCreateInfo vk2dInitGraphicsPipelineCreateInfo(VkPipelineShaderStageCreateInfo *shaderStages, uint32_t stageCount, VkPipelineVertexInputStateCreateInfo *vertexInputInfo, VkPipelineInputAssemblyStateCreateInfo *inputAssembly, VkPipelineViewportStateCreateInfo *viewport, VkPipelineRasterizationStateCreateInfo *rasterizer, VkPipelineMultisampleStateCreateInfo *multisampling, VkPipelineDepthStencilStateCreateInfo *depthStencil, VkPipelineColorBlendStateCreateInfo *colourBlending, VkPipelineDynamicStateCreateInfo *dynamicState, VkPipelineLayout layout, VkRenderPass renderPass) {
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = stageCount;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = vertexInputInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = inputAssembly;
	graphicsPipelineCreateInfo.pViewportState = viewport;
	graphicsPipelineCreateInfo.pRasterizationState = rasterizer;
	graphicsPipelineCreateInfo.pMultisampleState = multisampling;
	graphicsPipelineCreateInfo.pDepthStencilState = depthStencil;
	graphicsPipelineCreateInfo.pColorBlendState = colourBlending;
	graphicsPipelineCreateInfo.pDynamicState = dynamicState;
	graphicsPipelineCreateInfo.layout = layout;
	graphicsPipelineCreateInfo.renderPass = renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	return graphicsPipelineCreateInfo;
}

VkVertexInputBindingDescription vk2dInitVertexInputBindingDescription(VkVertexInputRate input, uint32_t stride, uint32_t binding) {
	VkVertexInputBindingDescription vertexInputBindingDescription = {};
	vertexInputBindingDescription.binding = binding;
	vertexInputBindingDescription.inputRate = input;
	vertexInputBindingDescription.stride = stride;
	return vertexInputBindingDescription;
}

VkVertexInputAttributeDescription vk2dInitVertexInputAttributeDescription(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset) {
	VkVertexInputAttributeDescription vertexInputAttributeDescription = {};
	vertexInputAttributeDescription.binding = binding;
	vertexInputAttributeDescription.format = format;
	vertexInputAttributeDescription.location = location;
	vertexInputAttributeDescription.offset = offset;
	return vertexInputAttributeDescription;
}

VkBufferCreateInfo vk2dInitBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, uint32_t *queueFamilies, uint32_t queueFamilyCount) {
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pQueueFamilyIndices = queueFamilies;
	bufferCreateInfo.queueFamilyIndexCount = queueFamilyCount;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.size = size;
	return bufferCreateInfo;
}

VkDescriptorSetLayoutBinding vk2dInitDescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType types, uint32_t typeCount, VkShaderStageFlags shaderStage, VkSampler *samplers) {
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
	descriptorSetLayoutBinding.binding = binding;
	descriptorSetLayoutBinding.descriptorCount = typeCount;
	descriptorSetLayoutBinding.descriptorType = types;
	descriptorSetLayoutBinding.pImmutableSamplers = samplers;
	descriptorSetLayoutBinding.stageFlags = shaderStage;
	return descriptorSetLayoutBinding;
}

VkDescriptorSetLayoutCreateInfo vk2dInitDescriptorSetLayoutCreateInfo(VkDescriptorSetLayoutBinding *bindings, uint32_t bindingCount) {
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pBindings = bindings;
	descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
	return descriptorSetLayoutCreateInfo;
}

VkDescriptorPoolCreateInfo vk2dInitDescriptorPoolCreateInfo(VkDescriptorPoolSize *sizes, uint32_t poolCount, uint32_t maxSets) {
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = poolCount;
	descriptorPoolCreateInfo.pPoolSizes = sizes;
	descriptorPoolCreateInfo.maxSets = maxSets;
	return descriptorPoolCreateInfo;
}

VkDescriptorSetAllocateInfo vk2dInitDescriptorSetAllocateInfo(VkDescriptorPool pool, uint32_t amount, VkDescriptorSetLayout *layouts) {
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = pool;
	descriptorSetAllocateInfo.descriptorSetCount = amount;
	descriptorSetAllocateInfo.pSetLayouts = layouts;
	return descriptorSetAllocateInfo;
}

VkSamplerCreateInfo vk2dInitSamplerCreateInfo(VkBool32 linearFilter, float anisotropy, float mipLevels) {
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = linearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = linearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = anisotropy > 1 ? VK_TRUE : VK_FALSE;
	samplerCreateInfo.maxAnisotropy = anisotropy;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.mipLodBias = 0;
	samplerCreateInfo.minLod = 0;
	samplerCreateInfo.maxLod = mipLevels;
	return samplerCreateInfo;
}

VkWriteDescriptorSet vk2dInitWriteDescriptorSet(VkDescriptorType type, uint32_t binding, VkDescriptorSet dstSet, VkDescriptorBufferInfo *info, uint32_t descriptorCount, VkDescriptorImageInfo *images) {
	VkWriteDescriptorSet writeDescriptorSet = {};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.descriptorCount = descriptorCount;
	writeDescriptorSet.pBufferInfo = info;
	writeDescriptorSet.pImageInfo = images;
	return writeDescriptorSet;
}