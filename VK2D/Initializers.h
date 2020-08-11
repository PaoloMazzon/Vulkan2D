/// \file Initializers.h
/// \author Paolo Mazzon
/// \brief Initializes Vulkan structs
#pragma once
#include "VK2D/Types.h"
#include <vulkan/vulkan.h>

VkApplicationInfo vk2dInitApplicationInfo(VK2DConfiguration *info);
VkInstanceCreateInfo vk2dInitInstanceCreateInfo(VkApplicationInfo *appInfo, const char **layers, int layerCount, const char **extensions, int extensionCount);
VkDeviceQueueCreateInfo vk2dInitDeviceQueueCreateInfo(uint32_t queueFamilyIndex, float *priority);
VkDeviceCreateInfo vk2dInitDeviceCreateInfo(VkDeviceQueueCreateInfo *info, uint32_t size, VkPhysicalDeviceFeatures *features);
VkCommandPoolCreateInfo vk2dInitCommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);
VkCommandBufferAllocateInfo vk2dInitCommandBufferAllocateInfo(VkCommandPool pool, uint32_t count);
VkDebugReportCallbackCreateInfoEXT vk2dInitDebugReportCallbackCreateInfoEXT(PFN_vkDebugReportCallbackEXT callback);
VkCommandBufferBeginInfo vk2dInitCommandBufferBeginInfo(VkCommandBufferUsageFlags flags);
VkSubmitInfo vk2dInitSubmitInfo(VkCommandBuffer *commandBufferVector, uint32_t commandBufferCount, VkSemaphore *signalSemaphoreVector, uint32_t signalSemaphoreCount, VkSemaphore *waitSemaphoreVector, uint32_t waitSemaphoreCount, VkPipelineStageFlags *waitFlag);
VkFenceCreateInfo vk2dInitFenceCreateInfo(VkFenceCreateFlags flags);
VkSemaphoreCreateInfo vk2dInitSemaphoreCreateInfo(VkSemaphoreCreateFlags flags);
VkSwapchainCreateInfoKHR vk2dInitSwapchainCreateInfoKHR(VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR capabilities, VkSurfaceFormatKHR format, uint32_t surfaceWidth, uint32_t surfaceHeight, VkPresentModeKHR presentMode, VkSwapchainKHR oldSwapchain, uint32_t minImageCount);
VkImageViewCreateInfo vk2dInitImageViewCreateInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectMask, uint32_t mipLevels);
VkImageCreateInfo vk2dInitImageCreateInfo(uint32_t imageWidth, uint32_t imageHeight, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels, VkSampleCountFlagBits samples);
VkMemoryAllocateInfo vk2dInitMemoryAllocateInfo(uint32_t size, uint32_t memoryType);
VkRenderPassCreateInfo vk2dInitRenderPassCreateInfo(VkAttachmentDescription *attachments, uint32_t attachCount, VkSubpassDescription *subpasses, uint32_t subpassCount, VkSubpassDependency *deps, uint32_t depCount);
VkFramebufferCreateInfo vk2dInitFramebufferCreateInfo(VkRenderPass renderPass, uint32_t surfaceWidth, uint32_t surfaceHeight, VkImageView *attachments, uint32_t attachCount);
VkPresentInfoKHR vk2dInitPresentInfoKHR(VkSwapchainKHR *swapchains, uint32_t swapchainCount, uint32_t *images, VkResult *presentResult, VkSemaphore *waitSemaphores, uint32_t semaphoreCount);
VkRenderPassBeginInfo vk2dInitRenderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer framebuffer, VkRect2D rect2D, VkClearValue *clearValues, uint32_t clearCount);
VkShaderModuleCreateInfo vk2dInitShaderModuleCreateInfo(char *code, uint32_t codeSize);
VkPipelineShaderStageCreateInfo vk2dInitPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shader);
VkPipelineVertexInputStateCreateInfo vk2dInitPipelineVertexInputStateCreateInfo(VkVertexInputBindingDescription *bindingDescriptions, uint32_t bindingDescriptionCount, VkVertexInputAttributeDescription *attributeDescriptions, uint32_t attributeDescriptionCount);
VkPipelineInputAssemblyStateCreateInfo vk2dInitPipelineInputAssemblyStateCreateInfo();
VkPipelineViewportStateCreateInfo vk2dInitPipelineViewportStateCreateInfo(VkViewport *viewport, VkRect2D *scissor);
VkPipelineRasterizationStateCreateInfo vk2dInitPipelineRasterizationStateCreateInfo();
VkPipelineMultisampleStateCreateInfo vk2dInitPipelineMultisampleStateCreateInfo(VkSampleCountFlagBits samples);
VkPipelineDepthStencilStateCreateInfo vk2dInitPipelineDepthStencilStateCreateInfo();
VkPipelineColorBlendStateCreateInfo vk2dInitPipelineColorBlendStateCreateInfo(VkPipelineColorBlendAttachmentState *attachments, uint32_t attachmentCount);
VkPipelineDynamicStateCreateInfo vk2dInitPipelineDynamicStateCreateInfo(VkDynamicState *states, uint32_t stateCount);
VkPipelineLayoutCreateInfo vk2dInitPipelineLayoutCreateInfo(VkDescriptorSetLayout *layouts, uint32_t layoutCount);
VkGraphicsPipelineCreateInfo vk2dInitGraphicsPipelineCreateInfo(VkPipelineShaderStageCreateInfo *shaderStages, uint32_t stageCount, VkPipelineVertexInputStateCreateInfo *vertexInputInfo, VkPipelineInputAssemblyStateCreateInfo *inputAssembly, VkPipelineViewportStateCreateInfo *viewport, VkPipelineRasterizationStateCreateInfo *rasterizer, VkPipelineMultisampleStateCreateInfo *multisampling, VkPipelineDepthStencilStateCreateInfo *depthStencil, VkPipelineColorBlendStateCreateInfo *colourBlending, VkPipelineDynamicStateCreateInfo *dynamicState, VkPipelineLayout layout, VkRenderPass renderPass);
VkVertexInputBindingDescription vk2dInitVertexInputBindingDescription(VkVertexInputRate input, uint32_t stride, uint32_t binding);
VkVertexInputAttributeDescription vk2dInitVertexInputAttributeDescription(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);
VkBufferCreateInfo vk2dInitBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, uint32_t *queueFamilies, uint32_t queueFamilyCount);
VkDescriptorSetLayoutBinding vk2dInitDescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType types, uint32_t typeCount, VkShaderStageFlags shaderStage, VkSampler *samplers);
VkDescriptorSetLayoutCreateInfo vk2dInitDescriptorSetLayoutCreateInfo(VkDescriptorSetLayoutBinding *bindings, uint32_t bindingCount);
VkDescriptorPoolCreateInfo vk2dInitDescriptorPoolCreateInfo(VkDescriptorPoolSize *sizes, uint32_t poolCount, uint32_t maxSets);
VkDescriptorSetAllocateInfo vk2dInitDescriptorSetAllocateInfo(VkDescriptorPool pool, uint32_t amount, VkDescriptorSetLayout *layouts);
VkSamplerCreateInfo vk2dInitSamplerCreateInfo(VkBool32 linearFilter, float anisotropy, float mipLevels);
VkWriteDescriptorSet vk2dInitWriteDescriptorSet(VkDescriptorType type, uint32_t binding, VkDescriptorSet dstSet, VkDescriptorBufferInfo *info, uint32_t descriptorCount, VkDescriptorImageInfo *images);