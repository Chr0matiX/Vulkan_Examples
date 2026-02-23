#include "vkContext.h"
#include "deviceVulkan.h"
#include "swapchainVulkan.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>

VkContext * VkContext::m_VkContextInstance{nullptr};

VkContext & VkContext::getInstance() {
	if (m_VkContextInstance == nullptr) {
		m_VkContextInstance = new VkContext();
		assert(m_VkContextInstance->init());
	}

	return *m_VkContextInstance;
}

bool VkContext::init() {
	bool rtn = false;

	do {
		VkInstanceCreateInfo vkInstanceCI{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		};

		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = m_AppName.c_str(),
			.pEngineName = m_Engine.c_str(),
			.apiVersion = m_ApiVersion,
		};
		vkInstanceCI.pApplicationInfo = &appInfo;

		std::vector<const char *> vec_EnableInstaceExtension;
		if (!getEnableInstaceExtension(vec_EnableInstaceExtension))
			break;
		vkInstanceCI.enabledExtensionCount =
			static_cast<uint32_t>(vec_EnableInstaceExtension.size());
		vkInstanceCI.ppEnabledExtensionNames = vec_EnableInstaceExtension.data();

		std::vector<const char *> vec_EnableInstanceLayer;
		if (!getEnableInstaceLayer(vec_EnableInstanceLayer))
			break;
		vkInstanceCI.enabledLayerCount = static_cast<uint32_t>(vec_EnableInstanceLayer.size());
		vkInstanceCI.ppEnabledLayerNames = vec_EnableInstanceLayer.data();

		// setDebugUtils(vkInstanceCI);

		VkDebugUtilsMessengerCreateInfoEXT debugMessCIExte{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
							   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
			.pfnUserCallback = debugUtilsMessCallback,
		};

		debugMessCIExte.pNext = vkInstanceCI.pNext;
		vkInstanceCI.pNext = &debugMessCIExte;

		// 此处还可以补充 layerSettings

		CHECK_VK_RESULT(vkCreateInstance(&vkInstanceCI, nullptr, &m_VkInstance));

		// surface
		{
			m_SurfaceVulkanInstance = new SurfaceVulkan();
			m_SurfaceVulkanInstance->m_AppInctance = GetModuleHandle(NULL);
			m_SurfaceVulkanInstance->m_VkInstance = m_VkInstance;
			m_SurfaceVulkanInstance->m_MainWindowsClassName = m_MainWindowsClassName;
			m_SurfaceVulkanInstance->m_WindowsTitle = m_WindowsTitle;

			if (!m_SurfaceVulkanInstance->init())
				break;
		}

		// device
		{
			m_DeviceVulkanInstance = new DeviceVulkan();
			m_DeviceVulkanInstance->m_VkInstance = m_VkInstance;
			m_DeviceVulkanInstance->m_SurfaceKHR = m_SurfaceVulkanInstance->m_SurfaceKHR;
			m_DeviceVulkanInstance->vec_ExpectDeviceExtension = vec_ExpectDeviceExtension;
			m_DeviceVulkanInstance->m_ExpectDeviceFeatures = m_ExpectDeviceFeatures;
			m_DeviceVulkanInstance->m_DefaultQueuePriority = m_DefaultQueuePriority;

			if (!m_DeviceVulkanInstance->init())
				break;
		}

		// swapchain
		{
			m_SwapchainVulkanInstance = new SwapchainVulkan();
			m_SwapchainVulkanInstance->m_MaxConcurrentFrames = m_MaxConcurrentFrames;
			m_SwapchainVulkanInstance->m_LogicalDevice = m_DeviceVulkanInstance->m_LogicalDevice;
			m_SwapchainVulkanInstance->m_PhysicalDevice = m_DeviceVulkanInstance->m_PhysicalDevice;
			m_SwapchainVulkanInstance->m_SurfaceKHR = m_SurfaceVulkanInstance->m_SurfaceKHR;
			m_SwapchainVulkanInstance->vec_ExpectCompositeAlphaFlags =
				vec_ExpectCompositeAlphaFlags;
			m_SwapchainVulkanInstance->vec_ExpectPreferredImageFormats =
				vec_ExpectPreferredImageFormats;
			m_SwapchainVulkanInstance->vec_DepthFormat = vec_DepthFormat;
			m_SwapchainVulkanInstance->vec_DepthStencilFormat = vec_DepthStencilFormat;
			m_SwapchainVulkanInstance->m_MemoryProperty = m_DeviceVulkanInstance->m_MemoryProperty;

			if (!m_SwapchainVulkanInstance->init())
				break;
		}

		// render
		{
			m_RenderVulkanInstance = new RenderVulkan();
			m_RenderVulkanInstance->vec_Vertex = {
				{{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
				{{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
				{{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
			};
			m_RenderVulkanInstance->vec_Index = {
				0,
				1,
				2,
			};
			m_RenderVulkanInstance->m_renderPass = m_SwapchainVulkanInstance->m_RenderPass;
			m_RenderVulkanInstance->m_QueueIndex = {
				.m_Graphics = m_DeviceVulkanInstance->m_QueueIndex.getGraphics(),
				.m_Present = m_DeviceVulkanInstance->m_QueueIndex.getPresent(),
				.m_Transfer = m_DeviceVulkanInstance->m_QueueIndex.getTransfer(),
				.m_Compute = m_DeviceVulkanInstance->m_QueueIndex.getCompute(),
			};
			m_RenderVulkanInstance->m_LogicalDevice = m_DeviceVulkanInstance->m_LogicalDevice;
			m_RenderVulkanInstance->m_MaxConcurrentFrames = m_MaxConcurrentFrames;
			m_RenderVulkanInstance->m_MemoryProperty = m_DeviceVulkanInstance->m_MemoryProperty;
			m_RenderVulkanInstance->map_QIndex2Queue = m_DeviceVulkanInstance->map_QIndex2Queue;
			m_RenderVulkanInstance->vec_ImageView = m_SwapchainVulkanInstance->vec_ImageView;
			m_RenderVulkanInstance->m_Swapchain = m_SwapchainVulkanInstance->m_SwapChain;
			m_RenderVulkanInstance->m_WindowWidth = m_SurfaceVulkanInstance->m_WindowWidth;
			m_RenderVulkanInstance->m_WindowHeight = m_SurfaceVulkanInstance->m_WindowHeight;
			m_RenderVulkanInstance->m_DepthStencilRes =
				m_SwapchainVulkanInstance->m_DepthStencilRes;

			if (!m_RenderVulkanInstance->init())
				break;
		}

		if (!valid())
			break;

		rtn = true;
	} while (0);

	return rtn;
}

VkBool32
VkContext::debugUtilsMessCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
								  VkDebugUtilsMessageTypeFlagsEXT messageType,
								  const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
								  void * pUserData) {
	return false; // do nothing
}

bool VkContext::valid() {
	return (m_VkContextInstance != nullptr) && (m_VkInstance != VK_NULL_HANDLE);
}

void VkContext::destroy() {
	if (m_SwapchainVulkanInstance != nullptr) {
		m_SwapchainVulkanInstance->destroy();
		delete m_SwapchainVulkanInstance;
		m_SwapchainVulkanInstance = nullptr;
	}

	if (m_DeviceVulkanInstance != nullptr) {
		m_DeviceVulkanInstance->destroy();
		delete m_DeviceVulkanInstance;
		m_DeviceVulkanInstance = nullptr;
	}

	if (m_SurfaceVulkanInstance != nullptr) {
		m_SurfaceVulkanInstance->destroy();
		delete m_SurfaceVulkanInstance;
		m_SurfaceVulkanInstance = nullptr;
	}

	if (m_VkInstance != VK_NULL_HANDLE) {
		vkDestroyInstance(m_VkInstance, nullptr);
		m_VkInstance = VK_NULL_HANDLE;
	}

	if (m_VkContextInstance != nullptr) {
		delete m_VkContextInstance;
		m_VkContextInstance = nullptr;
	}
}

bool VkContext::getEnableInstaceExtension(
	std::vector<const char *> & vec_EnableInstanceExtension) const {

	uint32_t extensionCount{0};
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	if (extensionCount <= 0) {
		std::cerr << "extensionCount is zero!\n";
		return false;
	}
	vec_EnableInstanceExtension.reserve(extensionCount);

	std::vector<VkExtensionProperties> vec_ExtensionProperty{extensionCount};
	CHECK_VK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
														   &vec_ExtensionProperty.front()));

	for (const auto & expectInstanceExtension : vec_ExpectInstanceExtension) {
		const auto & it_ExtensionProperty = std::find_if(
			vec_ExtensionProperty.begin(), vec_ExtensionProperty.end(),
			[&expectInstanceExtension](const VkExtensionProperties & extensionProperty) -> bool {
				return strcmp(extensionProperty.extensionName, expectInstanceExtension) == 0;
			});

		if (it_ExtensionProperty != vec_ExtensionProperty.end())
			vec_EnableInstanceExtension.emplace_back(expectInstanceExtension);
	}

	return !vec_EnableInstanceExtension.empty();
}

bool VkContext::getEnableInstaceLayer(std::vector<const char *> & vec_EnableInstanceLayer) const {

	uint32_t layerCount{0};
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	if (layerCount <= 0) {
		std::cerr << "layerCount is zero!\n";
		return false;
	}

	vec_EnableInstanceLayer.reserve(layerCount);

	std::vector<VkLayerProperties> vec_LayerProperty{layerCount};
	CHECK_VK_RESULT(vkEnumerateInstanceLayerProperties(&layerCount, vec_LayerProperty.data()));

	for (const auto & expectInstanceLayer : vec_ExpectInstanceLayer) {
		const auto & it_LayerProperty =
			std::find_if(vec_LayerProperty.begin(), vec_LayerProperty.end(),
						 [&expectInstanceLayer](const VkLayerProperties & layerProperty) -> bool {
							 return strcmp(expectInstanceLayer, layerProperty.layerName) == 0;
						 });

		if (it_LayerProperty != vec_LayerProperty.end())
			vec_EnableInstanceLayer.emplace_back(expectInstanceLayer);
	}

	return !vec_EnableInstanceLayer.empty();
}

/* void VkContext::setDebugUtils(VkInstanceCreateInfo & vkInstanceCI) const {
	VkDebugUtilsMessengerCreateInfoEXT debugMessCIExte{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
		.pfnUserCallback = debugUtilsMessCallback,
	};

	debugMessCIExte.pNext = vkInstanceCI.pNext;
	vkInstanceCI.pNext = &debugMessCIExte;

	return;
} */

void VkContext::startRenderLoop() {
	MSG msg;
	bool quitMessageReceived = false;

	while (!quitMessageReceived) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				quitMessageReceived = true;
				break;
			}
		}

		if (!IsIconic(m_SurfaceVulkanInstance->m_WindowHandle))
			m_RenderVulkanInstance->renderNext();
	}
}