#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <fstream>

#ifdef NDEBUG
	const bool enableValidationLayers = false;		// boolean flag for release and debug. if debug then set false
#else
	const bool enableValidationLayers = true;		// boolean flag for release and debug. if debug then set true
#endif // NDEBUG


const uint32_t WIDTH = 800;			//	window width
const uint32_t HEIGHT = 600;		//	window height

const int MAX_FRAMES_IN_FLIGHT = 2;	//	max frames count

// extenstions
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };	//	validation layer

// List of device extensions required by our renderer.
// Currently we only require the Swapchain extension.
// Without this extension, Vulkan cannot present rendered
// images to the window.
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

/* Creates a Vulkan Debug Messenger.
   Since vkCreateDebugUtilsMessengerEXT is part of an extension
   (not Vulkan Core), we must first retrieve its function pointer
   from the Vulkan driver before calling it. 
*/
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	// Ask Vulkan for the address of the extension function.
	// PFN_vkCreateDebugUtilsMessengerEXT is the correct
	// function-pointer type for vkCreateDebugUtilsMessengerEXT.
	auto func =
		(PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
			"vkCreateDebugUtilsMessengerEXT");

	// If the function exists, call it just like a normal function.
	if (func != nullptr)
	{
		return func(
			instance,
			pCreateInfo,
			pAllocator,
			pDebugMessenger);
	}
	else
	{
		// The extension was not enabled or is not supported.
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

/* Destroys the Vulkan Debug Messenger.
   Like the create function, this is also an extension function,
   so we must retrieve its address before calling it. 
*/
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	// Get the address of vkDestroyDebugUtilsMessengerEXT.
	auto func =
		(PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
			"vkDestroyDebugUtilsMessengerEXT");

	// If the function exists, destroy the debug messenger.
	if (func != nullptr)
	{
		func(
			instance,
			debugMessenger,
			pAllocator);
	}
}

/* Stores the indices of all queue families required by our renderer.
   Each member is std::optional because we don't know its value
   until we search the GPU's queue families. */
struct QueueFamilyIndices
{
	// Index of the queue family that supports Graphics commands. Initially empty until a suitable graphics queue is found.
	std::optional<uint32_t> graphicsFamily;
	// Index of the queue family that supports presenting commands. Initially empty until a suitable present queue is found.
	std::optional<uint32_t> presentFamily;
	/* 
		Check whether all required queue families have been found.
		Currently we require a Graphics Queue, present Queue.
		Later this function will also check for Compute,
		Transfer, and other queue families if needed. 

	*/

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();						// Returns true if graphicsFamily contains a valid index. Returns false if no graphics queue has been found yet.
	}
};

/* Stores all Swapchain information supported by a GPU
   for a particular window surface. */
struct SwapChainSupportDetails
{
	// General capabilities of the surface.
	// Examples:
	// - Minimum and maximum image count
	// - Current window size
	// - Supported image sizes
	// - Supported transforms
	VkSurfaceCapabilitiesKHR capabilities;

	// List of all supported image formats
	// (pixel format + color space).
	std::vector<VkSurfaceFormatKHR> formats;

	// List of all supported presentation modes
	// (FIFO, MAILBOX, IMMEDIATE, etc.).
	std::vector<VkPresentModeKHR> presentModes;
};


/* main class */
class HelloTriangleApplication {
public:
	/* Main Run Loop */
	void run() {
		initWindow();		//	create main window
		initVulkan();		//	initializing vulkan
		mainLoop();			//	main loop
		cleanup();			//	clean up delete all allocated memory
	}

private:

	// window
	GLFWwindow* window;									//	main window
	
	// vulkan instance
	VkInstance instance;								//	instance object
	VkDebugUtilsMessengerEXT debugMessenger;			//	Debuger object 
	VkSurfaceKHR surface;

	// devices
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;	//	Physical device object creation.
	VkDevice device;									//	logical device.

	// queue
	VkQueue graphicsQueue;								//	this stores the graphics queue data.
	VkQueue presentQueue;								//	this stores the present queue data.

	// swapchain
	VkSwapchainKHR swapChain;							//	this is main swapchain.
	std::vector<VkImage> swapChainImages;				//	stores and create image for swapchain.
	VkFormat swapChainImageFormat;						//	this stores the swapchain image format.
	VkExtent2D swapChainExtent;							//	this store the extend size.
	std::vector<VkImageView> swapChainImageViews;		//	this stores image view.

	// graphics
	VkRenderPass renderPass;							//	rendere pass.
	VkPipelineLayout pipelineLayout;					//	this stores pipline layout.
	VkPipeline graphicsPipeline;						//	this is main graphcis object.

	// framebuffer
	std::vector<VkFramebuffer> swapChainFramebuffers;	//	this store framebuffer data.

	// commands
	VkCommandPool commandPool;							//	this store command pool.
	std::vector<VkCommandBuffer> commandBuffers;						//	this store command buffer.

	// synchronous
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t currentFrame = 0;


	/* Initialize Window */
	void initWindow() {
		if (!glfwInit()) { std::cout << "Failed to Initialize GLFW ! " << std::endl; }	// initialize glew and also check that it was initialize correctly! best practice.

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);								// set no api as glfw was initially made for opengl and now using for vulkan required to set that value to false. it is done but glfwwindowhint function.
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);									// set resizable to flase as to make it resizable required some more code. for starting let it be false.

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);		// creates main window assign width and height then assign name for app then rest can be nullptr.

	}

	/* Initializing Vulkan */
	void initVulkan() {

		// Instance
		createInstance();															//	now we will call createInstance to create vulkan instance this is main part and step 1 for vulkan.
		setupDebugMessenger();														//	now also call setupDebugMessager to see actual error waring for code and it will be disable automatically in release mode.
		
		// window surface
		createSurface();															//	noew we will create an surface where we can present our rendering to screen.

		// Devices
		pickPhysicalDevice();														//	now we will call pickphysicaldevice function to get a best physical device that is present in systema and use it.
		createLogicalDevice();														//	now we will create and logical device.

		// swapchain
		createSwapChain();
		createImageViews();															//	now we will create an image view.
		
		// graphics pipline
		createRenderPass();															//	now we will render.
		createGraphicsPipeline();													//	now we will make our graphcis pipline.
		
		// framebuffer 
		createFramebuffers();														//	now we will create framebuufer
	
		// commands
		createCommandPool();														//	now we will create command pool
		createCommandBuffers();														//	now we will creat command buffer
		
		// synchronization 
		createSyncObjects();														//	now we will create an synchronization Objects.
	}

	/* Main Loop */
	void mainLoop() {

		/* run code until window is not close */
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();								// provide functionality like minimize resize and close window.
			
			drawFrame();
		}

		// wait for device to finish its work.
		vkDeviceWaitIdle(device);
	}

	/* clean up delets all memory allcated by program (required) */
	void cleanup() {

		// clear synchronization objects.
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		// clear command pool
		vkDestroyCommandPool(device, commandPool, nullptr);

		// destroy framebuffer
		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		// graphics
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		// swapchain images
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		// swapchain destroys.
		vkDestroySwapchainKHR(device, swapChain, nullptr);		//	destroy swapchain
		
		// delete device.
		vkDestroyDevice(device, nullptr);				// destroy created device.

		// delete debug message
		if (enableValidationLayers) {	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);	}		// if code run with debug mode then it needs to clear all allocation however in release mode it is not called.

		// delete window surface.
		vkDestroySurfaceKHR(instance, surface, nullptr);
		
		// delete instance of vulkan
		vkDestroyInstance(instance, nullptr);		// Destroy VULKAN INSTANCE 

		// delete window
		glfwDestroyWindow(window);					// GLFW WINDOW DESTROY
		
		// terminate program
		glfwTerminate();							// TERMINATE PROGRAM
	}

	/* create an Instance for vulkan */
	void createInstance() {

		if (enableValidationLayers && !checkValidationLayerSupport()) { throw std::runtime_error("validation layers requested, but not available!"); }		// check for validation layer if not found then throw error for it and it only happen when debug mode is on.

		// Application Info

		VkApplicationInfo appinfo{};							//  creating object for application.
		appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;		//	structure of object i.e application info.
		appinfo.pApplicationName = "Hello Triangle";			//	application name.
		appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);	//	application version.
		appinfo.pEngineName = "No Engine";						//	engine name.
		appinfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);		//	engine version.

		appinfo.apiVersion = VK_API_VERSION_1_0;				// api version.
		
		// Instance 

		VkInstanceCreateInfo createInfo{};								// CREATING OBJECT FRO INSTANCE.
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;		// CREATING STRUCTRUE FOR INSTANCE OBJECT.
		createInfo.pApplicationInfo = &appinfo;							// ASSIGN APPLICATION INFO WITH APPINFO.

		// EXTENSTION.

		uint32_t glfwExtensionCount = 0;											// glfwExtension Counter initialize with zero.
		const char** glfwExtensions;												// pointer to glfw Extension.

#if 0	/* starter code */

		// glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);	// provide glfwrequiredinstanceextension with counts i.e glfwExtensionCount = 0;
		
		// createInfo.enabledExtensionCount = glfwExtensionCount;					// assgin glfwcount to enabling vk instance object . propertie i.e enable extension count with get enable with value of count.
		// createInfo.ppEnabledExtensionNames = glfwExtensions;						// enable all extesntion with name provide from glfwExtensions.
		
		// createInfo.enabledLayerCount = 0;										// enable layer count with 0.
		
#endif

		auto extensions = getRequiredExtensions();										// this returns all required extensions.
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());	// enable extenstion count to extenstion size.
		createInfo.ppEnabledExtensionNames = extensions.data();							// enable extension name with extension data.

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};							//	created an object for debuginfo.
		if (enableValidationLayers)																	//	check is debug is on. If it is true then only run validation code or else no need on runtime.
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());			//	enable layer count with all layer that are avaible and provided by validation layer.
			createInfo.ppEnabledLayerNames = validationLayers.data();								//	assign enabled layer with name, which is provided by validation data function.
			
			populateDebugMessengerCreateInfo(debugCreateInfo);										//	for created debuginfo object we will populate it with messager.
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;				//	we will point to next debug info.
		}
		else
		{
			createInfo.enabledLayerCount = 0;														//	set layer count to 0 only if it not in debug as in release mode no need to check this validation any more.
		}


		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)	{ throw std::runtime_error("failed to create instance!"); }		// CREATED INSTANCE AND ALSO CHECK IS THAT WAS PERFECT. 
	}

	/*	Create a Vulkan Surface.
		A Surface connects the operating system's window (GLFW)
		with Vulkan so rendered images can be displayed on screen. 
	*/
	void createSurface()
	{
		// Ask GLFW to create a Vulkan-compatible surface
		// for our existing window.
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}


	/* Select a physical GPU from all Vulkan-compatible GPUs in the system. */
	void pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;																// Stores how many Vulkan-compatible GPUs are available.
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);							// First call: only retrieve the number of physical devices. Passing nullptr means "don't return the devices yet."

		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");				// No Vulkan-capable GPU was found.
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);										// Allocate enough space to store all GPU handles.

		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());						// Second call: retrieve all physical device handles.

		// Check every GPU until we find one that satisfies
		// the requirements of our renderer.
		for (const auto& device : devices)
		{
			if (isDeviceSuitable(device))
			{
				physicalDevice = device;														// Save the selected GPU.
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");							// Every GPU failed our suitability checks.
		}
	}

	/*	
		Create the Logical Device and all required queues.
		We now request both Graphics and Presentation queues.
		If both use the same queue family, we create only one queue. 
	*/
	void createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);				// Find the Graphics and Presentation queue families.

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;						// Store queue creation information.

		// Store only unique queue families.
		// If Graphics and Presentation are the same,
		// std::set automatically removes duplicates.
		std::set<uint32_t> uniqueQueueFamilies =
		{
			indices.graphicsFamily.value(),
			indices.presentFamily.value()
		};

		float queuePriority = 1.0f;

		// Create one queue description for every unique queue family.
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;											// Create one queue from this family.
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// No GPU features enabled yet.
		VkPhysicalDeviceFeatures deviceFeatures{};

		// Describe the Logical Device.
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;


#if 0	// before swapchain 
		createInfo.enabledExtensionCount = 0;
#endif
		// after swapchain
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

#if 0
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}
#endif

		// latest vulkan code
		createInfo.enabledLayerCount = 0;

		// Create the Logical Device.
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device!");
		}

		// Retrieve the Graphics Queue.
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);

		// Retrieve the Presentation Queue.
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	/* Create the Swapchain.
   The Swapchain stores the images that will be rendered
   and later presented (displayed) on the screen. */
	void createSwapChain()
	{
		// Query all Swapchain support information from the selected GPU.
		// This gives us:
		// - Surface capabilities
		// - Supported image formats
		// - Supported presentation modes
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		// Choose the best image format supported by the GPU.
		// Example:
		// VK_FORMAT_B8G8R8A8_SRGB
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);

		// Choose the best presentation mode.
		// Prefer MAILBOX, otherwise use FIFO.
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);

		// Choose the image size (width and height)
		// for every Swapchain image.
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		// Request one more image than the minimum supported.
		// This helps prevent the GPU from waiting while rendering.
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		// If the GPU specifies a maximum image count,
		// make sure we do not exceed it.
		if (swapChainSupport.capabilities.maxImageCount > 0 &&
			imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		// Create the Swapchain creation structure.
		VkSwapchainCreateInfoKHR createInfo{};

		// Specify that this structure describes a Swapchain.
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

		// The window surface where the Swapchain images
		// will eventually be presented.
		createInfo.surface = surface;

		// Number of images inside the Swapchain.
		createInfo.minImageCount = imageCount;

		// Pixel format for every Swapchain image.
		createInfo.imageFormat = surfaceFormat.format;

		// Color space used by the Swapchain images.
		createInfo.imageColorSpace = surfaceFormat.colorSpace;

		// Width and height of every Swapchain image.
		createInfo.imageExtent = extent;

		// Number of layers per image.
		// Normal applications always use one layer.
		createInfo.imageArrayLayers = 1;

		// Specify how the images will be used.
		// Here they are used as color attachments
		// because we will render directly into them.
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// Find the Graphics and Present queue families.
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		// Store both queue family indices.
		uint32_t queueFamilyIndices[] =
		{
			indices.graphicsFamily.value(),
			indices.presentFamily.value()
		};

		// Check whether Graphics and Present queues
		// belong to different queue families.
		if (indices.graphicsFamily != indices.presentFamily)
		{
			// Allow both queue families to access
			// the Swapchain images simultaneously.
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;

			// Two queue families will share the images.
			createInfo.queueFamilyIndexCount = 2;

			// Pass both queue family indices.
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			// Graphics and Present use the same queue family.
			// Exclusive mode gives better performance.
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		// Keep the current transformation of the window.
		// (Example: rotation or orientation supplied by the OS.)
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

		// Ignore the alpha channel when blending
		// the window with other windows.
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		// Selected presentation mode (MAILBOX or FIFO).
		createInfo.presentMode = presentMode;

		// We do not care about pixels that are hidden
		// behind another window. Vulkan may discard them
		// for better performance.
		createInfo.clipped = VK_TRUE;

		// No previous Swapchain exists.
		// Used later when recreating the Swapchain after resizing.
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		// Create the actual Swapchain.
		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		// --------------------------------------------
		// Retrieve all Swapchain Images
		// --------------------------------------------

		// First call:
		// Retrieve only the number of Swapchain images.
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);

		// Allocate enough space to store all image handles.
		swapChainImages.resize(imageCount);

		// Second call:
		// Retrieve the handles of every Swapchain image.
		vkGetSwapchainImagesKHR(
			device,
			swapChain,
			&imageCount,
			swapChainImages.data());

		// Save the chosen image format for later use
		// when creating Image Views and the Render Pass.
		swapChainImageFormat = surfaceFormat.format;

		// Save the image size for later use
		// during rendering.
		swapChainExtent = extent;
	}

	/* Create an Image View for every Swapchain Image.
   Vulkan cannot use a VkImage directly for rendering.
   An Image View describes how Vulkan should access that image. */
	void createImageViews()
	{
		// Allocate space for one Image View
		// for every Swapchain Image.
		swapChainImageViews.resize(swapChainImages.size());

		// Create an Image View for every image
		// inside the Swapchain.
		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			// Create the Image View description.
			VkImageViewCreateInfo createInfo{};

			// Specify that this structure describes
			// an Image View.
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

			// Select the Swapchain Image
			// this Image View will represent.
			createInfo.image = swapChainImages[i];

			// This is a normal 2D image.
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

			// Use the same image format
			// chosen when creating the Swapchain.
			createInfo.format = swapChainImageFormat;

			// Do not remap any color channels.
			// Keep R→R, G→G, B→B, A→A.
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			// This Image View represents the color aspect
			// of the image.
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			// Start from mip level 0.
			createInfo.subresourceRange.baseMipLevel = 0;

			// Use only one mip level.
			createInfo.subresourceRange.levelCount = 1;

			// Start from the first array layer.
			createInfo.subresourceRange.baseArrayLayer = 0;

			// Use only one array layer.
			createInfo.subresourceRange.layerCount = 1;

			// Create the Image View.
			if (vkCreateImageView(device, &createInfo, nullptr,
				&swapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	/* Create a Render Pass.
   A Render Pass describes:
   - Which images will be rendered to.
   - How those images should be handled before rendering.
   - How they should be handled after rendering.
   - Which subpasses will use those images. */
	void createRenderPass()
	{
		// ==========================================================
		// Color Attachment
		// This describes the Swapchain Image that we will render into.
		// ==========================================================
		VkAttachmentDescription colorAttachment{};

		// Image format must match the Swapchain Image format.
		colorAttachment.format = swapChainImageFormat;

		// Use one sample per pixel (No MSAA).
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		// Before rendering begins,
		// clear the image to the clear color.
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

		// After rendering,
		// keep the rendered image.
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		// We are not using a stencil buffer.
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		// Previous contents of the image are ignored.
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		// After rendering,
		// prepare the image for presentation on the screen.
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// ==========================================================
		// Attachment Reference
		// Tell the Subpass which attachment to use.
		// ==========================================================
		VkAttachmentReference colorAttachmentRef{};

		// Use attachment index 0.
		colorAttachmentRef.attachment = 0;

		// During rendering,
		// this attachment acts as a Color Attachment.
		colorAttachmentRef.layout =
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// ==========================================================
		// Subpass
		// A Subpass is one stage of rendering inside the Render Pass.
		// Here we create one Graphics Subpass.
		// ==========================================================
		VkSubpassDescription subpass{};

		// This Subpass uses the Graphics Pipeline.
		subpass.pipelineBindPoint =
			VK_PIPELINE_BIND_POINT_GRAPHICS;

		// One Color Attachment.
		subpass.colorAttachmentCount = 1;

		// Attach our Color Attachment Reference.
		subpass.pColorAttachments = &colorAttachmentRef;

		// ==========================================================
		// Render Pass Creation Info
		// Collect all attachments and subpasses together.
		// ==========================================================
		VkRenderPassCreateInfo renderPassInfo{};

		renderPassInfo.sType =
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

		// Total number of attachments.
		renderPassInfo.attachmentCount = 1;

		// Pointer to attachment descriptions.
		renderPassInfo.pAttachments = &colorAttachment;

		// Total number of Subpasses.
		renderPassInfo.subpassCount = 1;

		// Pointer to Subpass descriptions.
		renderPassInfo.pSubpasses = &subpass;

		// ==========================================================
		// Create the Vulkan Render Pass object.
		// ==========================================================
		if (vkCreateRenderPass(
			device,
			&renderPassInfo,
			nullptr,
			&renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}
	}
	
   /* Prepare all states required for creating the Graphics Pipeline.
  (The actual Graphics Pipeline will be created in a later chapter.) */
	void createGraphicsPipeline()
	{
		// ==========================================================
		// Read compiled SPIR-V shader files from disk.
		// ==========================================================
		auto vertShaderCode = readFile("shaders/vert.spv");   // Vertex Shader
		auto fragShaderCode = readFile("shaders/frag.spv");   // Fragment Shader

		// ==========================================================
		// Create Vulkan Shader Modules from the SPIR-V bytecode.
		// Shader Modules are Vulkan objects that store compiled shaders.
		// ==========================================================
		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		// ==========================================================
		// Vertex Shader Stage
		// Tell Vulkan which shader should execute during
		// the Vertex Shader stage.
		// ==========================================================
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;     // Vertex Shader stage
		vertShaderStageInfo.module = vertShaderModule;              // Shader Module to execute
		vertShaderStageInfo.pName = "main";                         // Entry function inside shader

		// ==========================================================
		// Fragment Shader Stage
		// Tell Vulkan which shader should execute during
		// the Fragment Shader stage.
		// ==========================================================
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;   // Fragment Shader stage
		fragShaderStageInfo.module = fragShaderModule;              // Shader Module
		fragShaderStageInfo.pName = "main";                         // Entry function

		// Store both shader stages together.
		// This array will later be passed to vkCreateGraphicsPipelines().
		VkPipelineShaderStageCreateInfo shaderStages[] =
		{
			vertShaderStageInfo,
			fragShaderStageInfo
		};

		// ==========================================================
		// Vertex Input State
		// Describes how vertex data is read from Vertex Buffers.
		//
		// Currently we are NOT using Vertex Buffers,
		// so there are no bindings or attributes.
		// ==========================================================
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;

		// ==========================================================
		// Input Assembly State
		// Describes how Vulkan should connect vertices together.
		// ==========================================================
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

		// Every 3 vertices become one triangle.
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// Primitive Restart is only useful for strips.
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// ==========================================================
		// Viewport State
		// Specifies how many Viewports and Scissor Rectangles
		// this pipeline will use.
		//
		// Actual viewport/scissor values are set dynamically later.
		// ==========================================================
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// ==========================================================
		// Rasterizer State
		// Converts triangles into fragments (pixels).
		// Also controls culling and polygon drawing mode.
		// ==========================================================
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

		// Don't clamp fragments beyond near/far planes.
		rasterizer.depthClampEnable = VK_FALSE;

		// Actually rasterize geometry.
		// If TRUE, nothing would be drawn.
		rasterizer.rasterizerDiscardEnable = VK_FALSE;

		// Draw filled triangles.
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

		// Line width (used when drawing lines).
		rasterizer.lineWidth = 1.0f;

		// Ignore back-facing triangles.
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

		// Clockwise vertices are considered the front face.
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

		// No depth bias.
		// Used later for Shadow Mapping.
		rasterizer.depthBiasEnable = VK_FALSE;

		// ==========================================================
		// Multisampling State
		// Controls Anti-Aliasing (MSAA).
		// ==========================================================
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

		// Disable sample shading.
		multisampling.sampleShadingEnable = VK_FALSE;

		// Use only one sample per pixel.
		// (No Anti-Aliasing)
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// ==========================================================
		// Color Blend Attachment
		// Describes how one framebuffer attachment
		// should receive color output.
		// ==========================================================
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};

		// Enable writing to all RGBA channels.
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;

		// Disable color blending.
		// New pixel completely replaces old pixel.
		colorBlendAttachment.blendEnable = VK_FALSE;

		// ==========================================================
		// Color Blend State
		// Controls blending for the entire pipeline.
		// ==========================================================
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		// Disable logical operations.
		colorBlending.logicOpEnable = VK_FALSE;

		// Copy output color directly.
		colorBlending.logicOp = VK_LOGIC_OP_COPY;

		// One framebuffer attachment.
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		// Blend constants.
		// Used only for certain blend modes.
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// ==========================================================
		// Dynamic States
		// These states will NOT be fixed inside the pipeline.
		// They can be changed during rendering using Vulkan commands.
		// ==========================================================
		std::vector<VkDynamicState> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		// ==========================================================
		// Pipeline Layout
		// Defines how shaders receive resources like:
		// - Uniform Buffers
		// - Textures
		// - Descriptor Sets
		// - Push Constants
		//
		// Currently we are not using any of these.
		// ==========================================================
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		// No Descriptor Set Layouts.
		pipelineLayoutInfo.setLayoutCount = 0;

		// No Push Constants.
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		// Create the Pipeline Layout object.
		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

		// ==========================================================
		// Graphics Pipeline Create Info
		// This structure combines every pipeline state that we
		// prepared earlier into one complete Graphics Pipeline.
		// ==========================================================
		VkGraphicsPipelineCreateInfo pipelineInfo{};

		// Tell Vulkan that this structure describes a Graphics Pipeline.
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		// Total number of shader stages.
		// We have one Vertex Shader and one Fragment Shader.
		pipelineInfo.stageCount = 2;

		// Pointer to the array containing shader stage descriptions.
		pipelineInfo.pStages = shaderStages;

		// Vertex Input configuration.
		// Describes how vertex data is read.
		pipelineInfo.pVertexInputState = &vertexInputInfo;

		// Input Assembly configuration.
		// Describes how vertices become primitives (triangles).
		pipelineInfo.pInputAssemblyState = &inputAssembly;

		// Viewport and Scissor configuration.
		pipelineInfo.pViewportState = &viewportState;

		// Rasterizer configuration.
		// Controls polygon mode, culling, line width, etc.
		pipelineInfo.pRasterizationState = &rasterizer;

		// Multisampling (MSAA) configuration.
		pipelineInfo.pMultisampleState = &multisampling;

		// Color Blending configuration.
		pipelineInfo.pColorBlendState = &colorBlending;

		// Dynamic States configuration.
		// Viewport and Scissor will be set during rendering.
		pipelineInfo.pDynamicState = &dynamicState;

		// Pipeline Layout.
		// Describes how shaders receive resources
		// (Descriptor Sets, Uniform Buffers, Push Constants).
		pipelineInfo.layout = pipelineLayout;

		// Render Pass this pipeline is compatible with.
		pipelineInfo.renderPass = renderPass;

		// Render Pass may contain multiple Subpasses.
		// This pipeline will be used in Subpass 0.
		pipelineInfo.subpass = 0;

		// No parent pipeline.
		// (Pipeline derivation is not used.)
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		// ==========================================================
		// Create the Graphics Pipeline.
		// Vulkan combines every state above into one VkPipeline object.
		// ==========================================================
		if (vkCreateGraphicsPipelines(
			device,
			VK_NULL_HANDLE,          // No Pipeline Cache
			1,                       // Create one pipeline
			&pipelineInfo,           // Pipeline description
			nullptr,                 // Default memory allocator
			&graphicsPipeline)       // Output Graphics Pipeline
			!= VK_SUCCESS)
		{
			throw std::runtime_error("failed to create graphics pipeline!");
		}


		// ==========================================================
		// Cleanup
		// Shader Modules are temporary objects.
		// After their information has been prepared,
		// they can be destroyed.
		// ==========================================================
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	/* Create Framebuffers.
   A Framebuffer is the actual collection of attachments (images)
   that the Render Pass will render into.

   Since every Swapchain Image has its own Image View,
   we must create one Framebuffer for each Swapchain Image.
*/
	void createFramebuffers()
	{

		// Resize the framebuffer vector so it can store
		// one framebuffer for every Swapchain Image View.
		swapChainFramebuffers.resize(swapChainImageViews.size());

		// Loop through every Swapchain Image View.
		for (size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			// ==========================================================
			// Attachments
			// These are the images that this framebuffer will use.
			// Currently we only have one Color Attachment.
			// ==========================================================
			VkImageView attachments[] =
			{
				swapChainImageViews[i]      // Current Swapchain Image View
			};

			// ==========================================================
			// Framebuffer Create Info
			// Describe how this framebuffer should be created.
			// ==========================================================
			VkFramebufferCreateInfo framebufferInfo{};

			// Tell Vulkan this is a Framebuffer Create structure.
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

			// This framebuffer must be compatible with this Render Pass.
			framebufferInfo.renderPass = renderPass;

			// Number of attachments used by this framebuffer.
			framebufferInfo.attachmentCount = 1;

			// Pointer to the attachment(s).
			framebufferInfo.pAttachments = attachments;

			// Width of the framebuffer.
			// Must match the Swapchain Image size.
			framebufferInfo.width = swapChainExtent.width;

			// Height of the framebuffer.
			framebufferInfo.height = swapChainExtent.height;

			// Number of image layers.
			// 1 means a normal 2D image.
			framebufferInfo.layers = 1;

			// ==========================================================
			// Create the Framebuffer.
			// Store it in the framebuffer array.
			// ==========================================================
			if (vkCreateFramebuffer(
				device,
				&framebufferInfo,
				nullptr,
				&swapChainFramebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	/* Create a Command Pool.
	   A Command Pool manages the memory used by Command Buffers.
	   Every Command Buffer is allocated from a Command Pool. */
	void createCommandPool()
	{
		// Find the Graphics Queue Family.
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		// Describe the Command Pool.
		VkCommandPoolCreateInfo poolInfo{};

		// Tell Vulkan this is a Command Pool structure.
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

		// Allow individual Command Buffers to be reset later.
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		// Command Buffers from this pool will execute on the Graphics Queue.
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		// Create the Command Pool.
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command pool!");
		}
	}

	/* Allocate one Primary Command Buffer from the Command Pool. */
	void createCommandBuffers()
	{
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};


		// Structure type.
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

		// Allocate from this Command Pool.
		allocInfo.commandPool = commandPool;

		// Create a Primary Command Buffer.
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		// Allocate one Command Buffer.
		//allocInfo.commandBufferCount = 1;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		// Allocate the Command Buffer.
		if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffers[currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	/* Create synchronization objects used every frame.
	   - imageAvailableSemaphore : Signaled when a Swapchain image is ready.
	   - renderFinishedSemaphore : Signaled when rendering is complete.
	   - inFlightFence           : Lets the CPU wait until the GPU finishes the previous frame.
	*/
	void createSyncObjects()
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		// ==========================================================
		// Semaphore Create Info
		// ==========================================================
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// ==========================================================
		// Fence Create Info
		// ==========================================================
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		// Start the fence in the signaled state so the first frame
		// doesn't block waiting for work that hasn't been submitted yet.
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		// Create the synchronization objects.
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}

	/* Render one frame. This function is called every iteration of the main loop. */
	void drawFrame()
	{
		// ==========================================================
		// Wait until the GPU finishes the previous frame.
		// ==========================================================
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		// Reset the fence so it can be signaled again when this frame finishes.
		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		// ==========================================================
		// Acquire the next available Swapchain image.
		// imageAvailableSemaphore will be signaled when it's ready.
		// ==========================================================
		uint32_t imageIndex;
		vkAcquireNextImageKHR(
			device,
			swapChain,
			UINT64_MAX,
			imageAvailableSemaphores[currentFrame],
			VK_NULL_HANDLE,
			&imageIndex);

		// ==========================================================
		// Reuse the Command Buffer by clearing previous commands.
		// ==========================================================
		vkResetCommandBuffer(commandBuffers[currentFrame], 0);

		// Record drawing commands for the acquired Swapchain image.
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		// ==========================================================
		// Describe how the GPU should execute this work.
		// ==========================================================
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// GPU waits until the Swapchain image becomes available.
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };

		// Wait before writing to the color attachment.
		VkPipelineStageFlags waitStages[] =
		{
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		// Execute this Command Buffer.
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		// Signal this semaphore after rendering finishes.
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// Submit commands to the Graphics Queue.
		// When execution completes, the fence will be signaled.
		if (vkQueueSubmit(
			graphicsQueue,
			1,
			&submitInfo,
			inFlightFences[currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		// ==========================================================
		// Present the rendered image.
		// ==========================================================
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		// Wait until rendering is complete before presenting.
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		// Present to this Swapchain.
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		// Present the acquired image.
		presentInfo.pImageIndices = &imageIndex;

		// Display the image on the screen.
		vkQueuePresentKHR(presentQueue, &presentInfo);
	}
	/* Record all rendering commands into the Command Buffer. */
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		// ==========================================================
		// Begin recording commands.
		// ==========================================================
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		// ==========================================================
		// Begin Render Pass.
		// ==========================================================
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		// Use our Render Pass.
		renderPassInfo.renderPass = renderPass;

		// Render into the current Swapchain Framebuffer.
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

		// Render to the entire framebuffer.
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		// Clear framebuffer to opaque black.
		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		// Record command: Begin Render Pass.
		vkCmdBeginRenderPass(
			commandBuffer,
			&renderPassInfo,
			VK_SUBPASS_CONTENTS_INLINE);

		// ==========================================================
		// Bind the Graphics Pipeline.
		// ==========================================================
		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphicsPipeline);

		// ==========================================================
		// Set Viewport dynamically.
		// ==========================================================
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainExtent.width);
		viewport.height = static_cast<float>(swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		// ==========================================================
		// Set Scissor Rectangle dynamically.
		// ==========================================================
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// ==========================================================
		// Draw one triangle.
		// 3 vertices
		// 1 instance
		// Start from vertex 0
		// First instance = 0
		// ==========================================================
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		// ==========================================================
		// Finish the Render Pass.
		// ==========================================================
		vkCmdEndRenderPass(commandBuffer);

		// ==========================================================
		// Finish recording commands.
		// ==========================================================
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}
	}
	/* Create a Vulkan Shader Module from
   compiled SPIR-V shader bytecode. */
	VkShaderModule createShaderModule(const std::vector<char>& code)
	{
		// Describe the Shader Module.
		VkShaderModuleCreateInfo createInfo{};

		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		// Size of the SPIR-V bytecode.
		createInfo.codeSize = code.size();

		// Pointer to the shader bytecode.
		createInfo.pCode =
			reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;

		// Create the Shader Module.
		if (vkCreateShaderModule(
			device,
			&createInfo,
			nullptr,
			&shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}


	/* Read a binary SPIR-V shader file from disk
   and return its contents as a byte array. */
	static std::vector<char> readFile(const std::string& filename)
	{
		// Open the file in binary mode and position
		// the file pointer at the end of the file.
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		// Check whether the file opened successfully.
		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file!");
		}

		// Determine the size of the file.
		size_t fileSize = (size_t)file.tellg();

		// Allocate enough memory to store the file.
		std::vector<char> buffer(fileSize);

		// Move the file pointer back to the beginning.
		file.seekg(0);

		// Read the entire file into memory.
		file.read(buffer.data(), fileSize);

		// Close the file.
		file.close();

		// Return the binary shader code.
		return buffer;
	}


	/* Choose the best image format for the Swapchain.
   The GPU may support many formats, so we search for our
   preferred one: BGRA8 with the SRGB color space.
   If it is not available, we simply use the first format
   supported by the GPU. */
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		// Check every format supported by the GPU.
		for (const auto& availableFormat : availableFormats)
		{
			// Our preferred format:
			// - BGRA (Blue, Green, Red, Alpha)
			// - 8 bits per color channel
			// - SRGB color space for correct color output
			if (availableFormat.format ==
				VK_FORMAT_B8G8R8A8_SRGB &&
				availableFormat.colorSpace ==
				VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				// Preferred format found.
				return availableFormat;
			}
		}

		// Preferred format was not found.
		// Return the first format supported by the GPU.
		// It may not be ideal, but it is guaranteed to be compatible.
		return availableFormats[0];
	}

	/* Choose the best presentation mode for the Swapchain.
   Presentation mode controls how rendered images are
   displayed on the monitor.

   We prefer MAILBOX because it provides:
   - Low latency
   - Smooth animation
   - No screen tearing

   If MAILBOX is unavailable, we use FIFO because it is
   guaranteed to be supported by every Vulkan implementation.
*/
	VkPresentModeKHR chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		// Check every presentation mode supported by the GPU.
		for (const auto& availablePresentMode : availablePresentModes)
		{
			// Prefer MAILBOX mode for the best gaming experience.
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		// MAILBOX is not supported.
		// Use FIFO, which is always available and prevents screen tearing.
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	/* Choose the size (width and height) of the Swapchain images.
   Usually this matches the window's framebuffer size.
   Some platforms provide a fixed size, while others allow
   the application to choose within supported limits. */
	VkExtent2D chooseSwapExtent(
		const VkSurfaceCapabilitiesKHR& capabilities)
	{
		// If the operating system has already chosen a fixed
		// framebuffer size, simply use it.
		if (capabilities.currentExtent.width !=
			std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			// The application must choose the image size.

			int width, height;

			// Retrieve the current framebuffer size from GLFW.
			glfwGetFramebufferSize(window, &width, &height);

			// Convert the framebuffer size into Vulkan's VkExtent2D type.
			VkExtent2D actualExtent =
			{
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			// Clamp the width so it stays within the limits
			// supported by the GPU.
			actualExtent.width = std::clamp(
				actualExtent.width,
				capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width);

			// Clamp the height so it stays within the limits
			// supported by the GPU.
			actualExtent.height = std::clamp(
				actualExtent.height,
				capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height);

			// Return the final image size.
			return actualExtent;
		}
	}

	/* Query everything needed to create a Swapchain.
   This function asks the GPU which surface capabilities,
   image formats, and presentation modes are supported. */
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		// Structure that will store all supported information.
		SwapChainSupportDetails details;

		// Retrieve general surface capabilities.
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
			device,
			surface,
			&details.capabilities);

		// ---------- Surface Formats ----------

		uint32_t formatCount;

		// First call:
		// Retrieve only the number of supported formats.
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device,
			surface,
			&formatCount,
			nullptr);

		if (formatCount != 0)
		{
			// Allocate memory for every supported format.
			details.formats.resize(formatCount);

			// Second call:
			// Retrieve all supported formats.
			vkGetPhysicalDeviceSurfaceFormatsKHR(
				device,
				surface,
				&formatCount,
				details.formats.data());
		}

		// ---------- Present Modes ----------

		uint32_t presentModeCount;

		// First call:
		// Retrieve only the number of supported present modes.
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device,
			surface,
			&presentModeCount,
			nullptr);

		if (presentModeCount != 0)
		{
			// Allocate memory for every supported present mode.
			details.presentModes.resize(presentModeCount);

			// Second call:
			// Retrieve all supported presentation modes.
			vkGetPhysicalDeviceSurfacePresentModesKHR(
				device,
				surface,
				&presentModeCount,
				details.presentModes.data());
		}

		// Return all collected Swapchain support information.
		return details;
	}


	/* 
		Check whether a GPU satisfies our requirements.
	*/
	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = findQueueFamilies(device);					//	get indices for all queue family.
		bool extensionsSupported = checkDeviceExtensionSupport(device);			//	check if it support extenstion.
		return indices.isComplete() && extensionsSupported;						//	return if isCompleted and extesntion is supported.
	}

	/* Check whether the selected GPU supports every device
   extension required by our renderer. */
	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;

		// First call:
		// Retrieve only the number of supported extensions.
		vkEnumerateDeviceExtensionProperties(
			device,
			nullptr,
			&extensionCount,
			nullptr);

		// Allocate memory for all supported extension properties.
		std::vector<VkExtensionProperties>
			availableExtensions(extensionCount);

		// Second call:
		// Retrieve information about every supported extension.
		vkEnumerateDeviceExtensionProperties(
			device,
			nullptr,
			&extensionCount,
			availableExtensions.data());

		// Create a set containing every extension that
		// our application requires.
		std::set<std::string> requiredExtensions(
			deviceExtensions.begin(),
			deviceExtensions.end());

		// Remove every extension that the GPU actually supports.
		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		// If the set is empty, every required extension
		// was found on this GPU.
		return requiredExtensions.empty();
	}

	/*	
		Search all queue families of the selected GPU and find
		the queue families required by our renderer. 
	*/
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;										// Stores the indices of the required queue families.

		uint32_t queueFamilyCount = 0;

		// First call: retrieve only the number of queue families.
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		// Allocate memory for all queue family properties.
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

		// Second call: retrieve all queue family properties.
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;

		// Check every queue family.
		for (const auto& queueFamily : queueFamilies)
		{
			// Does this queue family support Graphics commands?
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			// Check whether this queue family can present
			// rendered images to our window surface.
			VkBool32 presentSupport = false;

			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport)
			{
				indices.presentFamily = i;
			}

			// Stop searching once both required queue families
			// have been found.
			if (indices.isComplete())
			{
				break;
			}

			i++;
		}

		return indices;
	}


	/* setup debug messager */
	void setupDebugMessenger() {
		if (!enableValidationLayers) return;				//	return if it in release mode else it will execute.

		VkDebugUtilsMessengerCreateInfoEXT createInfo;		//	create object for debug utils messager 
		populateDebugMessengerCreateInfo(createInfo);		//	this call populate debug messgaer create info this populate all msg for program 

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) { throw std::runtime_error("failed to set up debug messenger!"); }		// now create debug utils messager if this not return true then throw error.
	}

	/* Fill the Debug Messenger CreateInfo structure.
	   This structure tells Vulkan what messages we want
	   and which callback function should receive them. */
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};																// Clear the entire structure. Sets all members to 0 or nullptr to avoid garbage values.

		createInfo.sType =	VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;	// Every Vulkan structure must specify its type. This identifies the structure as a Debug Messenger CreateInfo.
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		/*		Select which message severities we want to receive :
				Verbose  -> Detailed information.
				Warning  -> Potential problems.
				Error    -> Serious mistakes.
				The '|' operator combines multiple flags together.	
		*/

		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		/*		Select which categories of messages we want :
				General     -> General Vulkan information.
				Validation  -> Incorrect Vulkan API usage.
				Performance -> Suggestions for better performance.
		*/


		createInfo.pfnUserCallback = debugCallback;											// Register our callback function. Whenever Vulkan generates a selected debug message,	it will automatically call debugCallback().
	}
	
	/* Returns all Vulkan instance extensions required by GLFW.
	   If validation layers are enabled, also adds the Debug Utils extension. */
	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;												// Stores how many Vulkan instance extensions GLFW requires
		const char** glfwExtensions;													// Pointer to an array of extension name strings returned by GLFW.

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);		// GLFW fills glfwExtensionCount with the number of required extensions and returns a pointer to their names.

		// Copy the extension names from GLFW's array into a std::vector so we can easily add more extensions later.
		std::vector<const char*> extensions(
			glfwExtensions,
			glfwExtensions + glfwExtensionCount
		);

		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);					// Add the Debug Utils extension.This is required for creating a Debug Messenger.
		}

		return extensions;																// Return the final list of required Vulkan instance extensions.
	}

	/* it will return all requested layers */
	bool checkValidationLayerSupport() {
		
		uint32_t layerCount;															//	counter for layercount i.e initilizally set to 0.
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);						//	it list all available layers.

		std::vector<VkLayerProperties> availableLayers(layerCount);						//	this store all avaiable layer counts so with that we can get its properties of that all layers.
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());		//	this get all properties of layers.

		for (const char* layerName : validationLayers) {								// we will do an for loop for layername now this will check for all validationlayer if we found then we will set bool layerfound to true else false with this we can get layername 
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {						// another loop for layerproperties with avaiablelayers
				if (strcmp(layerName, layerProperties.layerName) == 0) {				// checks if layername and layerproperties.layername are equal then set bool layerfound to true or else to false.
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;															// return false as we have not found layer
			}
		}

		return true;																	// return true
	}

	/* This Function Simple Show debug information into console */
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}



};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch(const std::exception& e){
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}