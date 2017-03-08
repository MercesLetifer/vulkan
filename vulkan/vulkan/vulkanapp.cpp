#include "vulkanapp.h"
#include <stdexcept>

void VulkanApp::run()
{
	initWindow();
	mainLoop();
}

VulkanApp::~VulkanApp()
{
	cleanup();
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

void VulkanApp::mainLoop()
{
	while (!glfwWindowShouldClose(window_)) 
		glfwPollEvents();
}

void VulkanApp::cleanup()
{

}
