#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

class VulkanApp {
	GLFWwindow*	window_ = nullptr;
	VkInstance instance_ = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;

	struct {					// struct for application info
		int WIDTH = 800;
		int HEIGHT = 600;
		const char* title = "Vulkan application";
	} info_;

private:
	void initWindow();
	void mainLoop();
	void cleanup();

public:
	void run();
	~VulkanApp();

};