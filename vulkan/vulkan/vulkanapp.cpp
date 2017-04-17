#include "vulkanapp.h"
#include <stdexcept>
#include <vector>
#include <iostream>		
#include <fstream>
#include <numeric>

VkResult CreateDebugReportCallbackEXT(VkInstance instance, 
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, 
	VkDebugReportCallbackEXT* pCallback) 
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, 
		"vkCreateDebugReportCallbackEXT");

	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pCallback);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, 
	const VkAllocationCallbacks* pAllocator) 
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, 
		"vkDestroyDebugReportCallbackEXT");

	if (func != nullptr)
		func(instance, callback, pAllocator);
}


void VulkanApp::initAppInfo()
{
	if (info_.enableValidationLayers)
		info_.instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");

	uint32_t extensionCount = 0;
	auto extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	for (uint32_t i = 0; i < extensionCount; ++i)
		info_.instanceExtensions.push_back(*(extensions + i));
	info_.instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
}

void VulkanApp::run()
{
	initWindow();
	initAppInfo();		// rename function
	initVulkan();

	showInfo();			// for help

	mainLoop();
}

void VulkanApp::initWindow()
{
	if (!glfwInit())
		throw std::runtime_error("failed to init glfw library");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window_ = glfwCreateWindow(info_.WIDTH, info_.HEIGHT, info_.title, nullptr, nullptr);
	if (!window_)
		throw std::runtime_error("failed to create window");

	glfwSetWindowUserPointer(window_, this);
	glfwSetWindowSizeCallback(window_, VulkanApp::onWindowResized);
}

void VulkanApp::initVulkan()
{
	createInstance();

	if (info_.enableValidationLayers)
		setupDebugCallback();

	createSurface();
	pickPhysicalDevice();
	createDevice();
	
	createSwapchain();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createVertexBuffer();
	createCommandBuffers();
	createSemaphores();
}

void VulkanApp::createInstance()
{
	checkInstanceLayersSupport();
	checkInstanceExtenstionsSupport();

	VkApplicationInfo appInfo = { };
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = info_.title;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = info_.instanceLayers.size();
	createInfo.ppEnabledLayerNames = info_.instanceLayers.data();
	createInfo.enabledExtensionCount = info_.instanceExtensions.size();
	createInfo.ppEnabledExtensionNames = info_.instanceExtensions.data();

	if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS)
		throw std::runtime_error("failed to create instance");
}

void VulkanApp::pickPhysicalDevice()
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance_, &physicalDeviceCount, nullptr);

	if (!physicalDeviceCount)
		throw std::runtime_error("failed to find GPU with Vulkan support");

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance_, &physicalDeviceCount, physicalDevices.data());

	for (const auto& device : physicalDevices) {
		if (checkDeviceExtensionSupport(device)) {
			auto familyIndices = getFamilyIndices(device);
			if (familyIndices.graphicFamily >= 0 && familyIndices.presentFamily >= 0) {
				physicalDevice_ = device;
				return;
			}
		}
	}

	// if no devices with extension and surface support throw exception
	throw std::runtime_error("failed to pick suitable device!");
}

void VulkanApp::createDevice()
{
	float queuePripority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	auto familyIndices = getFamilyIndices(physicalDevice_);
	VkDeviceQueueCreateInfo deviceGraphicQueueCreateInfo = {};
	deviceGraphicQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceGraphicQueueCreateInfo.queueFamilyIndex = (uint32_t)familyIndices.graphicFamily;
	deviceGraphicQueueCreateInfo.queueCount = 1;
	deviceGraphicQueueCreateInfo.pQueuePriorities = &queuePripority;
	queueCreateInfos.push_back(deviceGraphicQueueCreateInfo);
	
	if (familyIndices.graphicFamily != familyIndices.presentFamily) {
		VkDeviceQueueCreateInfo devicePresentQueueCreateInfo = { };
		devicePresentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		devicePresentQueueCreateInfo.queueFamilyIndex = (uint32_t)familyIndices.presentFamily;
		devicePresentQueueCreateInfo.queueCount = 1;
		devicePresentQueueCreateInfo.pQueuePriorities = &queuePripority;
		queueCreateInfos.push_back(devicePresentQueueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo = { };
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledLayerCount = 0;			
	deviceCreateInfo.enabledExtensionCount = info_.deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = info_.deviceExtensions.data();
	deviceCreateInfo.pEnabledFeatures = nullptr;

	if (vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr, &device_) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");

	vkGetDeviceQueue(device_, familyIndices.graphicFamily, 0, &graphicQueue_);
	vkGetDeviceQueue(device_, familyIndices.presentFamily, 0, &presentQueue_);
}

void VulkanApp::createSurface()
{
	if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface");
}

void VulkanApp::createSwapchain()
{
	auto capabilities = getSurfaceCapabilities();
	auto format = getSurfaceFormat();
	auto presentMode = getPresentMode();	

 	VkSwapchainCreateInfoKHR createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface_;
	createInfo.minImageCount = capabilities.minImageCount + 1;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.imageExtent = capabilities.currentExtent;	
	createInfo.imageArrayLayers = 1;						
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto indices = getFamilyIndices(physicalDevice_);
	uint32_t familyIndeces[] = { (uint32_t)indices.graphicFamily, (uint32_t)indices.presentFamily };
	if (indices.graphicFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = familyIndeces;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	VkSwapchainKHR oldSwapchain = swapchain_;
	createInfo.oldSwapchain = oldSwapchain;

	VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
	if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &newSwapchain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swapchain");

	swapchain_ = newSwapchain;

	// get swapchain images
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
	std::vector<VkImage> swapchainImages(imageCount);
	vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages.data());

	// create swapchain image views
	imageViews_.resize(imageCount, VK_NULL_HANDLE);

	for (uint32_t i = 0; i < imageCount; ++i) {
		VkImageViewCreateInfo imageViewCreateInfo = { };
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = swapchainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = format.format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device_, &imageViewCreateInfo, nullptr, &imageViews_[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create image view");
	}
}

void VulkanApp::createRenderPass()
{
	auto format = getSurfaceFormat();

	VkAttachmentDescription colorAttachment = { };
	colorAttachment.format = format.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = { };
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = { };
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency subpassDependency = { };
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpassDescription;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &subpassDependency;

	if (vkCreateRenderPass(device_, &createInfo, nullptr, &renderPass_) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass");
}

void VulkanApp::createGraphicsPipeline()
{
	auto vertexShaderCode = readFile(info_.vertexFile);
	auto fragmentShaderCode = readFile(info_.fragmentFile);

	VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
	VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;

	createShaderModule(vertexShaderCode, vertexShaderModule);
	createShaderModule(fragmentShaderCode, fragmentShaderModule);

	VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = { };
	vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageCreateInfo.module = vertexShaderModule;
	vertexShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = { };
	fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageCreateInfo.module = fragmentShaderModule;
	fragmentShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo };

	// add vertices in shader
	VkVertexInputBindingDescription vertexBindingDescription = { };
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(Vertex);
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription posAttributeDescription = { };
	posAttributeDescription.binding = 0;
	posAttributeDescription.location = 0;
	posAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
	posAttributeDescription.offset = offsetof(Vertex, pos);

	VkVertexInputAttributeDescription colorAttributeDescription = { };
	colorAttributeDescription.binding = 0;
	colorAttributeDescription.location = 1;
	colorAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	colorAttributeDescription.offset = offsetof(Vertex, color);

	VkVertexInputAttributeDescription attributes[] = { posAttributeDescription, colorAttributeDescription };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = { };
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = 2;
	vertexInputInfo.pVertexAttributeDescriptions = attributes;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = { };
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = { };
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)info_.WIDTH;
	viewport.height = (float)info_.HEIGHT;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = { };
	scissor.offset = { 0, 0 };
	scissor.extent = { (uint32_t)info_.WIDTH, (uint32_t)info_.HEIGHT };

	VkPipelineViewportStateCreateInfo viewportInfo = { };
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports = &viewport;
	viewportInfo.scissorCount = 1;
	viewportInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = { };
	rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationCreateInfo.depthBiasClamp = 0.0f;
	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationCreateInfo.lineWidth = 1.0f;
	
	VkPipelineMultisampleStateCreateInfo multisampleInfo = { };
	multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleInfo.sampleShadingEnable = VK_FALSE;
	multisampleInfo.minSampleShading = 1.0f;
	multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleInfo.alphaToOneEnable = VK_FALSE;
	
	VkPipelineColorBlendAttachmentState colorBlendAttachment = { };
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendInfo = { };
	colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = &colorBlendAttachment;
	colorBlendInfo.blendConstants[0] = 0.0f;
	colorBlendInfo.blendConstants[1] = 0.0f;
	colorBlendInfo.blendConstants[2] = 0.0f;
	colorBlendInfo.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;	

	if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = shaderStages;
	createInfo.pVertexInputState = &vertexInputInfo;
	createInfo.pInputAssemblyState = &inputAssemblyInfo;
	createInfo.pViewportState = &viewportInfo;
	createInfo.pRasterizationState = &rasterizationCreateInfo;
	createInfo.pMultisampleState = &multisampleInfo;
	createInfo.pColorBlendState = &colorBlendInfo;
	createInfo.layout = pipelineLayout_;
	createInfo.renderPass = renderPass_;
	createInfo.subpass = 0;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &createInfo, nullptr, &graphicPipeline_) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphic pipeline!");

	// destroy shader modules
	vkDestroyShaderModule(device_, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(device_, vertexShaderModule, nullptr);
}

void VulkanApp::createCommandPool()
{
	auto indices = getFamilyIndices(physicalDevice_);

	VkCommandPoolCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = indices.graphicFamily;

	if (vkCreateCommandPool(device_, &createInfo, nullptr, &commandPool_) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");
}

void VulkanApp::createFramebuffers()
{
	framebuffers_.resize(imageViews_.size(), VK_NULL_HANDLE);

	for (size_t i = 0; i < framebuffers_.size(); ++i) {
		VkImageView attachments[] = { imageViews_[i] };

		VkFramebufferCreateInfo createInfo = { };
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderPass_;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;
		createInfo.width = info_.WIDTH;
		createInfo.height = info_.HEIGHT;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(device_, &createInfo, nullptr, &framebuffers_[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}
}

void VulkanApp::createCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool_;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t) framebuffers_.size();

	commandBuffers_.resize(framebuffers_.size(), VK_NULL_HANDLE);
	if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffers!");

	for (size_t i = 0; i < commandBuffers_.size(); ++i) {
		VkCommandBufferBeginInfo beginInfo = { };
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(commandBuffers_[i], &beginInfo);

		VkRenderPassBeginInfo renderpassBeginInfo = { };
		renderpassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpassBeginInfo.renderPass = renderPass_;
		renderpassBeginInfo.framebuffer = framebuffers_[i];
		renderpassBeginInfo.renderArea.offset = { 0, 0 };
		renderpassBeginInfo.renderArea.extent = { (uint32_t)info_.WIDTH, (uint32_t)info_.HEIGHT };
		renderpassBeginInfo.clearValueCount = 1;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderpassBeginInfo.pClearValues = &clearColor;
		
		vkCmdBeginRenderPass(commandBuffers_[i], &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipeline_);

		VkBuffer vertexBuffers[] = { vertexBuffer_ };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers_[i], 0, 1, vertexBuffers, offsets);

		vkCmdDraw(commandBuffers_[i], vertices.size(), 1, 0, 0);

		vkCmdEndRenderPass(commandBuffers_[i]);

		if (vkEndCommandBuffer(commandBuffers_[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to record commands in command buffer!");
	}
}

void VulkanApp::createSemaphores()
{
	VkSemaphoreCreateInfo imageAvailableInfo = { };
	imageAvailableInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphoreCreateInfo renderFinishedInfo = { };
	renderFinishedInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device_, &imageAvailableInfo, nullptr, &imageAvailableSemaphore_) != VK_SUCCESS ||
		vkCreateSemaphore(device_, &renderFinishedInfo, nullptr, &renderFinishedSemaphore_) != VK_SUCCESS)
		throw std::runtime_error("failed to create semaphore");
}

void VulkanApp::drawFrame()
{
	uint32_t imageIndex = 0;
	vkAcquireNextImageKHR(device_, swapchain_, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore_, 0, &imageIndex);

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore_ };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = { };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore_ };

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicQueue_, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		throw std::runtime_error("failed to submit command buffer!");

	VkPresentInfoKHR presentInfo = { };
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapchains[] = { swapchain_ };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	
	vkQueuePresentKHR(presentQueue_, &presentInfo);
}

void VulkanApp::mainLoop()
{
	while (!glfwWindowShouldClose(window_)) {
		glfwPollEvents();
		drawFrame();
	}		
	
	glfwDestroyWindow(window_);
	glfwTerminate();
}

// help functions
VulkanApp::FamilyIndices VulkanApp::getFamilyIndices(VkPhysicalDevice device)
{
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> familyProperties(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, familyProperties.data());

	FamilyIndices familyIndices = { };
	for (uint32_t index = 0; index < familyProperties.size(); ++index) {
		if (familyProperties[index].queueCount > 0) {
			if (familyProperties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				familyIndices.graphicFamily = index;

			VkBool32 supported = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface_, &supported);
			if (supported)
				familyIndices.presentFamily = index;

			if (familyIndices.graphicFamily >= 0 && familyIndices.presentFamily >= 0)
				return familyIndices;
		}
	}

	// generate an exception if graphics and present families not supported
	throw std::runtime_error("failed to find suitable queue families");
}

void VulkanApp::checkInstanceLayersSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> layers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

	for (const auto& l : info_.instanceLayers) {
		for (auto beg = layers.begin(); beg != layers.cend(); ++beg) {
			if (strcmp(l, beg->layerName) == 0)
				break;

			if (beg + 1 == layers.cend()) {
				std::string errorStr = "instance don't support"
					+ std::string(l) + " layer!";
				throw std::runtime_error(errorStr);
			}	
		}
	}
}

void VulkanApp::checkInstanceExtenstionsSupport()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	for (const auto& ie : info_.instanceExtensions) {
		for (auto beg = extensions.cbegin(); beg != extensions.cend(); ++beg) {
			if (strcmp(beg->extensionName, ie) == 0)
				break;

			if (beg + 1 == extensions.cend()) {
				std::string errorStr = "instance don't support " 
					+ std::string(ie) + " extension";
				throw std::runtime_error(errorStr);
			}
		}
	}
}

bool VulkanApp::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties>extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	for (const auto& de : info_.deviceExtensions) {
		for (auto beg = extensions.cbegin(); beg != extensions.cend(); ++beg) {
			if (strcmp(beg->extensionName, de) == 0)
				break;
			
			if (beg + 1 == extensions.cend())
				return false;
		}
	}

	return true;
}

void VulkanApp::setupDebugCallback()
{
	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (CreateDebugReportCallbackEXT(instance_, &createInfo, nullptr, &callback_) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}

std::vector<char> VulkanApp::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::in | std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("failed to open file!");

	size_t length = (size_t)file.tellg();					// or use function gcount?
	std::vector<char> buffer(length);
	file.seekg(0);

	file.read(buffer.data(), length);
	file.close();

	return buffer;
}

void VulkanApp::createShaderModule(const std::vector<char>& code, VkShaderModule& mod)
{
	// TODO: ALIGNMENT!!!
	VkShaderModuleCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = (uint32_t*)code.data();

	if (vkCreateShaderModule(device_, &createInfo, nullptr, &mod) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");
}

uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		if (typeFilter & (1 << i) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties)		// check
			return i;
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VkSurfaceCapabilitiesKHR VulkanApp::getSurfaceCapabilities()
{
	VkSurfaceCapabilitiesKHR capabilities = { };
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &capabilities);

	return capabilities;
}

VkSurfaceFormatKHR VulkanApp::getSurfaceFormat()
{
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data());

	if (formats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (const auto& f : formats) {
		if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return f;
	}

	return formats[0];	// for now
}

VkPresentModeKHR VulkanApp::getPresentMode()
{
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, presentModes.data());

	for (const auto& m : presentModes) {
		if (m == VK_PRESENT_MODE_MAILBOX_KHR)
			return m;
	}

	return presentModes[0];	
}

// delete functions
void VulkanApp::cleanup()
{
	vkDeviceWaitIdle(device_);

	if (vertexBufferMemory_) {
		vkFreeMemory(device_, vertexBufferMemory_, nullptr);
		vertexBufferMemory_ = VK_NULL_HANDLE;
	}

	if (vertexBuffer_) {
		vkDestroyBuffer(device_, vertexBuffer_, nullptr);
		vertexBuffer_ = VK_NULL_HANDLE;
	}

	if (imageAvailableSemaphore_) {
		vkDestroySemaphore(device_, imageAvailableSemaphore_, nullptr);
		imageAvailableSemaphore_ = VK_NULL_HANDLE;
	}

	if (renderFinishedSemaphore_) {
		vkDestroySemaphore(device_, renderFinishedSemaphore_, nullptr);
		renderFinishedSemaphore_ = VK_NULL_HANDLE;
	}

	for (auto& framebuffer : framebuffers_) {
		if (framebuffer) {
			vkDestroyFramebuffer(device_, framebuffer, nullptr);
			framebuffer = VK_NULL_HANDLE;
		}
	}

	if (graphicPipeline_) {
		vkDestroyPipeline(device_, graphicPipeline_, nullptr);
		graphicPipeline_ = VK_NULL_HANDLE;
	}

	if (pipelineLayout_) {
		vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
		pipelineLayout_ = VK_NULL_HANDLE;
	}

	if (renderPass_) {
		vkDestroyRenderPass(device_, renderPass_, nullptr);
		renderPass_ = VK_NULL_HANDLE;
	}

	for (auto& imageView : imageViews_) {
		if (imageView) {
			vkDestroyImageView(device_, imageView, nullptr);
			imageView = VK_NULL_HANDLE;
		}	
	}

	if (swapchain_) {
		vkDestroySwapchainKHR(device_, swapchain_, nullptr);
		swapchain_ = VK_NULL_HANDLE;
	}

	if (commandPool_) {
		vkDestroyCommandPool(device_, commandPool_, nullptr);
		commandPool_ = VK_NULL_HANDLE;
	}

	if (device_) {
		vkDestroyDevice(device_, nullptr);
		device_ = VK_NULL_HANDLE;
	}

	if (callback_) {
		DestroyDebugReportCallbackEXT(instance_, callback_, nullptr);
		callback_ = VK_NULL_HANDLE;
	}

	if (surface_) {
		vkDestroySurfaceKHR(instance_, surface_, nullptr);
		surface_ = VK_NULL_HANDLE;
	}

	if (instance_) {
		vkDestroyInstance(instance_, nullptr);
		instance_ = VK_NULL_HANDLE;
	}
}

void VulkanApp::recreateSwapchain()
{
	vkDeviceWaitIdle(device_);

	int width = 0, height = 0;
	glfwGetWindowSize(window_, &width, &height);
	info_.WIDTH = width;
	info_.HEIGHT = height;

	if (!commandBuffers_.empty()) {
		vkFreeCommandBuffers(device_, commandPool_, commandBuffers_.size(), commandBuffers_.data());
		commandBuffers_.clear();
	}

	for (auto& framebuffer : framebuffers_) {
		if (framebuffer) {
			vkDestroyFramebuffer(device_, framebuffer, nullptr);
			framebuffer = VK_NULL_HANDLE;
		}
	}

	if (graphicPipeline_) {
		vkDestroyPipeline(device_, graphicPipeline_, nullptr);
		graphicPipeline_ = VK_NULL_HANDLE;
	}

	if (pipelineLayout_) {
		vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
		pipelineLayout_ = VK_NULL_HANDLE;
	}

	if (renderPass_) {
		vkDestroyRenderPass(device_, renderPass_, nullptr);
		renderPass_ = VK_NULL_HANDLE;
	}

	for (auto& imageView : imageViews_) {
		if (imageView) {
			vkDestroyImageView(device_, imageView, nullptr);
			imageView = VK_NULL_HANDLE;
		}
	}

	if (swapchain_) {
		vkDestroySwapchainKHR(device_, swapchain_, nullptr);
		swapchain_ = VK_NULL_HANDLE;
	}

	createSwapchain();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandBuffers();
}

void VulkanApp::onWindowResized(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0) return;

	VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
	app->recreateSwapchain();
}

void VulkanApp::createVertexBuffer()
{
	VkBufferCreateInfo bufferInfo = { };
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(vertices[0]) * vertices.size();
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device_, &bufferInfo, nullptr, &vertexBuffer_) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer!");

	VkMemoryRequirements memoryRequiremets;
	vkGetBufferMemoryRequirements(device_, vertexBuffer_, &memoryRequiremets);

	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequiremets.size;
	allocInfo.memoryTypeIndex = findMemoryType(memoryRequiremets.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(device_, &allocInfo, nullptr, &vertexBufferMemory_) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate vertex buffer memory!");

	vkBindBufferMemory(device_, vertexBuffer_, vertexBufferMemory_, 0);

	void* data = nullptr;
	vkMapMemory(device_, vertexBufferMemory_, 0, bufferInfo.size, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferInfo.size);
	vkUnmapMemory(device_, vertexBufferMemory_);

}

VulkanApp::~VulkanApp()
{
	cleanup();
}


VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApp::debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
	std::cerr << "Validation layer: " << msg << std::endl;

	return VK_FALSE;
}

// delete after relise
void VulkanApp::showInfo()
{
	//--------------show instance layers-------------------------
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> layerProperties(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

	std::cout << "Instance layer names\n";
	for (const auto& p : layerProperties)
		std::cout << p.layerName << '\n';
	std::cout << std::endl;

	//--------------show instance extensions-----------------------
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);	
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		
	std::cout << "Instance extension names\n";
	for (const auto& ext : extensions)
		std::cout << ext.extensionName << '\n';
	std::cout << std::endl;

	//--------------show device layers and extensions---------------
	if (physicalDevice_) {
		vkEnumerateDeviceLayerProperties(physicalDevice_, &layerCount, nullptr);
		layerProperties.resize(layerCount);
		vkEnumerateDeviceLayerProperties(physicalDevice_, &layerCount, layerProperties.data());

		std::cout << "Device layer names\n";
		for (const auto& l : layerProperties)
			std::cout << l.layerName << '\n';
		std::cout << std::endl;

		vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extensionCount, nullptr);
		extensions.resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extensionCount, extensions.data());

		std::cout << "Device extension names\n";
		for (const auto& e : extensions)
			std::cout << e.extensionName << '\n';
		std::cout << std::endl;

		// physical device properties
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice_, &physicalDeviceProperties);

		std::cout << "Physical device properties\n";
		std::cout << "Device name: " << physicalDeviceProperties.deviceName
			<< "\nMax viewports: " << physicalDeviceProperties.limits.maxViewports
			<< "\nMax viewport dimension: " << physicalDeviceProperties.limits.maxViewportDimensions[0]
			<< "x" << physicalDeviceProperties.limits.maxViewportDimensions[1] 
			<< "\nMax color attachments: " << physicalDeviceProperties.limits.maxColorAttachments
			<< "\nMax vertex input bindings: " << physicalDeviceProperties.limits.maxVertexInputBindings
			<< "\nMax vertex input binding stride: " << physicalDeviceProperties.limits.maxVertexInputBindingStride
			<< "\nMax vertex input attributes: " << physicalDeviceProperties.limits.maxVertexInputAttributes
			<< std::endl;
		std::cout << std::endl;

		// queue family properties
		uint32_t queueFamilyPropertyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyPropertyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyPropertyCount, queueFamilyProperties.data());

		std::cout << "Queue family properties";
		for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i) {
			std::cout << "\nIndex: " << i
				<< "\nQueue count: " << queueFamilyProperties[i].queueCount
				<< "\nTimestamp valid bits: " << queueFamilyProperties[i].timestampValidBits
				<< "\nFlags: ";

			auto queueFlag = queueFamilyProperties[i].queueFlags;
			if (queueFlag & VK_QUEUE_GRAPHICS_BIT)			std::cout << "-graphic ";
			if (queueFlag & VK_QUEUE_COMPUTE_BIT)			std::cout << "-compute ";
			if (queueFlag & VK_QUEUE_TRANSFER_BIT)			std::cout << "-transfer ";
			if (queueFlag & VK_QUEUE_SPARSE_BINDING_BIT)	std::cout << "-sparse ";

			auto imageGran = queueFamilyProperties[i].minImageTransferGranularity;
			std::cout << "\nMin image transfer granularity(WxHxD): "
				<< imageGran.width << 'x' << imageGran.height << 'x' << imageGran.depth;

			std::cout << std::endl;
		}
	}
}

