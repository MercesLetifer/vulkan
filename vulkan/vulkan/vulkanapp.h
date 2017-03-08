#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

class VulkanApp {
	GLFWwindow*	window_ = nullptr;
	VkInstance instance_ = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
	VkDevice device_ = VK_NULL_HANDLE;

	struct {					// struct for application info
		int WIDTH = 800;
		int HEIGHT = 600;
		const char* title = "Vulkan application";
	} info_;

private:
	void initWindow();		// main functions
	void createInstance();
	void pickPhysicalDevice();
	void createDevice();
	void mainLoop();
	void cleanup();

private:		// help functions
	uint32_t getFamilyIndex();

public:
	void run();
	~VulkanApp();

};