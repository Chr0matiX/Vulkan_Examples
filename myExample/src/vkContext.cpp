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
		VkInstanceCreateInfo vkInstanceCI{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

		VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
								  .pApplicationName = m_AppName,
								  .pEngineName = m_Engine,
								  .apiVersion = m_ApiVersion};
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

		setDebugUtils(vkInstanceCI);

		// 此处还可以补充 layerSettings

		CHECK_VK_RESULT(vkCreateInstance(&vkInstanceCI, nullptr, &m_VkInstance));

		// surface
		{
			m_SurfaceVulkanInstance = new SurfaceVulkan();
			m_SurfaceVulkanInstance->m_AppInctance = GetModuleHandle(NULL);
			m_SurfaceVulkanInstance->m_VkInstance = m_VkInstance;
			strcpy(m_SurfaceVulkanInstance->m_MainWindowsClassName, m_MainWindowsClassName);
			strcpy(m_SurfaceVulkanInstance->m_WindowsTitle, m_WindowsTitle);

			m_SurfaceVulkanInstance->init();
			if (!m_SurfaceVulkanInstance->valid())
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

			m_DeviceVulkanInstance->init();
			if (!m_DeviceVulkanInstance->valid())
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

			m_SwapchainVulkanInstance->init();
			if (!m_SwapchainVulkanInstance->valid())
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
			vec_EnableInstanceExtension.emplace_back(it_ExtensionProperty->extensionName);
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
			vec_EnableInstanceLayer.emplace_back(it_LayerProperty->layerName);
	}

	return !vec_EnableInstanceLayer.empty();
}

void VkContext::setDebugUtils(VkInstanceCreateInfo & vkInstanceCI) const {
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
}
