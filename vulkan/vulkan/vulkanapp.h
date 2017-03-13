#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>
#include <vector>

class VulkanApp {
	GLFWwindow*	window_ = nullptr;
	VkInstance instance_ = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
	VkDevice device_ = VK_NULL_HANDLE;
	VkSurfaceKHR surface_ = VK_NULL_HANDLE;
	VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;

	struct {					// struct for application info
		int WIDTH = 800;
		int HEIGHT = 600;
		const char* title = "Vulkan application";

		std::vector<const char*> instanceExtensions;

		std::vector<const char*> deviceExtensions{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

	} info_;

private:
	void initAppInfo();
	void initWindow();		// main functions
	void createInstance();
	void pickPhysicalDevice();
	void createDevice();
	void createSurface();
	void createSwapchain();
	void mainLoop();
	void cleanup();

private:		// help functions
	uint32_t getFamilyIndex();
	void checkInstanceExtenstionSupport();
	void checkDeviceExtensionSupport(VkPhysicalDevice);
	// functions for creating swap chain
	VkSurfaceCapabilitiesKHR getSurfaceCapabilities();
	VkSurfaceFormatKHR getSurfaceFormat();
	VkPresentModeKHR getPresentMode();

	void showInfo();		// super help function for me, delete after relise

public:
	void run();
	~VulkanApp();

};