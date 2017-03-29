#include "vulkanapp.h"
#include <stdexcept>
#include <vector>
#include <iostream>		

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

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window_ = glfwCreateWindow(info_.WIDTH, info_.HEIGHT, info_.title, nullptr, nullptr);
	if (!window_)
		throw std::runtime_error("failed to create window");

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
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_) != VK_SUCCESS)
		throw std::runtime_error("failed to create swapchain");

	// get swapchain images
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
	std::vector<VkImage> swapchainImages(imageCount);
	vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages.data());

	// create swapchain image views
	imageViews_.resize(imageCount);

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

void VulkanApp::mainLoop()
{
	while (!glfwWindowShouldClose(window_)) 
		glfwPollEvents();
	
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

VulkanApp::~VulkanApp()
{
	cleanup();
}


VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApp::debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char * layerPrefix, const char * msg, void * userData)
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
			<< "x" << physicalDeviceProperties.limits.maxViewportDimensions[1] << std::endl;
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
			std::cout << std::endl;
		}
	}
}

