#include "vulkanapp.h"
#include <stdexcept>
#include <vector>

void VulkanApp::run()
{
	initWindow();
	createInstance();
	pickPhysicalDevice();
	createDevice();

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
	createInfo.enabledExtensionCount = 0;		// no extensions

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
	deviceCreateInfo.enabledExtensionCount = 0;		// no extensions
	deviceCreateInfo.pEnabledFeatures = nullptr;

	if (vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr, &device_) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");
}

void VulkanApp::mainLoop()
{
	while (!glfwWindowShouldClose(window_)) 
		glfwPollEvents();
	
	glfwDestroyWindow(window);
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

// delete functions
void VulkanApp::cleanup()
{
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
