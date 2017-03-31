#ifndef VULKANAPP_H_
#define VULKANAPP_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>
#include <vector>
#include <string>

class VulkanApp {
	GLFWwindow*	window_ = nullptr;
	VkInstance instance_ = VK_NULL_HANDLE;
	VkSurfaceKHR surface_ = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT callback_;
	VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
	VkDevice device_ = VK_NULL_HANDLE;
	VkQueue graphicQueue_ = VK_NULL_HANDLE;
	VkQueue presentQueue_ = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
	std::vector<VkImageView> imageViews_;
	VkRenderPass renderPass_ = VK_NULL_HANDLE;

	struct {					// struct for application info
		int WIDTH = 800;
		int HEIGHT = 600;
		const char* title = "Vulkan application";

		// layers and extensions
		std::vector<const char*> instanceLayers;
		std::vector<const char*> instanceExtensions;

		std::vector<const char*> deviceExtensions{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		// flags
#ifdef NDEBUG
		bool enableValidationLayers = false;
#else
		bool enableValidationLayers = true;
#endif

	} info_;

	struct FamilyIndices {
		int32_t graphicFamily = -1;
		int32_t presentFamily = -1;
	};

private:
	void initAppInfo();
	void initWindow();		// main functions
	void initVulkan();

	void createInstance();	// init vulkan functions
	void pickPhysicalDevice();
	void createDevice();
	void createSurface();
	void createSwapchain();
	void createRenderPass();
	void createGraphicsPipeline();

	void mainLoop();
	void cleanup();

private:		// help functions
	FamilyIndices getFamilyIndices(VkPhysicalDevice device);
	void checkInstanceLayersSupport();
	void checkInstanceExtenstionsSupport();
	bool checkDeviceExtensionSupport(VkPhysicalDevice);
	void setupDebugCallback();
	std::vector<char> readFile(const std::string& filename);
	
	// functions for creating swap chain
	VkSurfaceCapabilitiesKHR getSurfaceCapabilities();
	VkSurfaceFormatKHR getSurfaceFormat();
	VkPresentModeKHR getPresentMode();

	// debug functions
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code,
		const char* layerPrefix, const char* msg, void* userData);

	void showInfo();		// super help function for me, delete after relise

public:
	void run();
	~VulkanApp();

};

#endif // VULKANAPP_H_