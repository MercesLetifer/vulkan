#include "vulkanapp.h"
#include <stdexcept>
#include <vector>
#include <iostream>		// for help

void VulkanApp::initAppInfo()
{
	uint32_t extensionCount = 0;
	auto extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	for (uint32_t i = 0; i < extensionCount; ++i)
		info_.instanceExtensions.push_back(*(extensions + i));

	// add desired extensions with info_.instanceExtensions.push_back(desiredExtension)
}

void VulkanApp::run()
{
	initWindow();
	initAppInfo();		// rename function

	createInstance();
	pickPhysicalDevice();
	createDevice();
	createSurface();
	createSwapchain();

	showInfo();			// 

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

void VulkanApp::createInstance()
{
	checkInstanceExtenstionSupport();

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
	createInfo.enabledLayerCount = 0;		// no layers
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

	checkDeviceExtensionSupport(physicalDevices[0]);

	physicalDevice_ = physicalDevices[0];		// now pick first physical device
}

void VulkanApp::createDevice()
{
	VkDeviceQueueCreateInfo deviceQueueCreateInfo = { };
	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.queueFamilyIndex = getFamilyIndex();
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = nullptr;	// default priority

	VkDeviceCreateInfo deviceCreateInfo = { };
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.enabledLayerCount = 0;			// no layers
	deviceCreateInfo.enabledExtensionCount = info_.deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = info_.deviceExtensions.data();
	deviceCreateInfo.pEnabledFeatures = nullptr;

	if (vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr, &device_) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");
}

void VulkanApp::createSurface()
{
	if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface");
}

void VulkanApp::createSwapchain()
{
	// TODO: rewrite
	VkSurfaceCapabilitiesKHR surfaceCapabilities = { };
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &surfaceCapabilities);
	
 	VkSwapchainCreateInfoKHR createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface_;
	createInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
	createInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	createInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	createInfo.imageExtent = surfaceCapabilities.currentExtent;	
	createInfo.imageArrayLayers = 1;						
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = surfaceCapabilities.currentTransform;		// no transform
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain");
}

void VulkanApp::mainLoop()
{
	while (!glfwWindowShouldClose(window_)) 
		glfwPollEvents();
	
	glfwDestroyWindow(window_);
	glfwTerminate();
}

// help functions
uint32_t VulkanApp::getFamilyIndex()
{
	// TODO: rewrite
	uint32_t queueFamilyPropertyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyPropertyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyPropertyCount, 
		queueFamilyProperties.data());

	for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
		auto fp = queueFamilyProperties[i];
		
		if (fp.queueCount > 0 && fp.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			return i;
	}

	// if function is not find need queue family when it's generate an exception
	throw std::runtime_error("failed to get need queue family");
//	return -1;
}

void VulkanApp::checkInstanceExtenstionSupport()
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

void VulkanApp::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties>extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	for (const auto& de : info_.deviceExtensions) {
		for (auto beg = extensions.cbegin(); beg != extensions.cend(); ++beg) {
			if (strcmp(beg->extensionName, de) == 0)
				break;
			
			if (beg + 1 == extensions.cend()) {
				std::string errorStr = "device don't support "
					+ std::string(de) + " extension";
				throw std::runtime_error(errorStr);
			}
		}
	}
}

// delete functions
void VulkanApp::cleanup()
{
	if (swapChain_) {
		vkDestroySwapchainKHR(device_, swapChain_, nullptr);
		swapChain_ = VK_NULL_HANDLE;
	}

	if (surface_) {
		vkDestroySurfaceKHR(instance_, surface_, nullptr);
		surface_ = VK_NULL_HANDLE;
	}

	if (device_) {
		vkDestroyDevice(device_, nullptr);
		device_ = VK_NULL_HANDLE;
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

	//--------------show device layers and properties---------------
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
	}
}