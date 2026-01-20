#include "vulkanManager.h"

#include "deviceManager.h"
#include "surfaceManager.h"

#include <algorithm>
#include <cassert>
#include <iostream>

CVkManager * CVkManager::m_VkManagerInstance{nullptr};

CVkManager & CVkManager::getInstance() {
	if (m_VkManagerInstance == nullptr) {
		m_VkManagerInstance = new CVkManager();
		assert(m_VkManagerInstance->initManager());
	}

	return *m_VkManagerInstance;
}

bool CVkManager::initManager() {
	bool rtn = false;

	do {
		m_AppInctance = GetModuleHandle(NULL);

		// Create VkInstace
		{
			VkInstanceCreateInfo instanceCI{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

			///
			VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
									  .pApplicationName = m_AppName,
									  .pEngineName = m_Engine,
									  .apiVersion = m_ApiVersion};
			instanceCI.pApplicationInfo = &appInfo;

			///
			std::vector<const char *> vec_EnableInstExtension;
			{
				uint32_t extensionCount{0};
				vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
				if (extensionCount <= 0) {
					std::cerr << "extensionCount is zero!\n";
					break;
				}

				vec_EnableInstExtension.reserve(extensionCount);

				std::vector<VkExtensionProperties> vec_ExtensionProperty{extensionCount};
				if (vkEnumerateInstanceExtensionProperties(
						nullptr, &extensionCount, &vec_ExtensionProperty.front()) != VK_SUCCESS) {
					std::cerr << "EnumerateInstanceExtensionProperties failed!\n";
					break;
				}

				// for (const auto & extProp : vec_ExtProp)
				//	vec_EnableInstExte.emplace_back(extProp.extensionName);

				for (const auto & expectInstanceExtension : vec_ExpectInstanceExtension) {
					const auto & it_ExtensionProperty =
						std::find_if(vec_ExtensionProperty.begin(), vec_ExtensionProperty.end(),
									 [&expectInstanceExtension](
										 const VkExtensionProperties & extensionProperty) -> bool {
										 return strcmp(extensionProperty.extensionName,
													   expectInstanceExtension) == 0;
									 });

					if (it_ExtensionProperty != vec_ExtensionProperty.end())
						vec_EnableInstExtension.emplace_back(it_ExtensionProperty->extensionName);
				}
			}
			if (!vec_EnableInstExtension.empty()) {
				instanceCI.enabledExtensionCount =
					static_cast<uint32_t>(vec_EnableInstExtension.size());
				instanceCI.ppEnabledExtensionNames = vec_EnableInstExtension.data();
			} else {
				std::cerr << "EnableInstExte is emtyp!\n";
				break;
			}

			///
			std::vector<const char *> vec_EnableInstanceLayer;
			{
				uint32_t layerCount{0};
				vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
				if (layerCount <= 0) {
					std::cerr << "layerCount is zero!\n";
					break;
				}

				vec_EnableInstanceLayer.reserve(layerCount);

				std::vector<VkLayerProperties> vec_LayerProperty{layerCount};
				if (vkEnumerateInstanceLayerProperties(&layerCount, vec_LayerProperty.data()) !=
					VK_SUCCESS) {
					std::cerr << "EnumerateInstanceLayerProperties failed!\n";
					break;
				}

				for (const auto & expectInstanceLayer : vec_ExpectInstanceLayer) {
					const auto & it_LayerProperty = std::find_if(
						vec_LayerProperty.begin(), vec_LayerProperty.end(),
						[&expectInstanceLayer](const VkLayerProperties & layerProperty) -> bool {
							return strcmp(expectInstanceLayer, layerProperty.layerName) == 0;
						});

					if (it_LayerProperty != vec_LayerProperty.end())
						vec_EnableInstanceLayer.emplace_back(it_LayerProperty->layerName);
				}
			}
			if (!vec_EnableInstanceLayer.empty()) {
				instanceCI.enabledLayerCount =
					static_cast<uint32_t>(vec_EnableInstanceLayer.size());
				instanceCI.ppEnabledLayerNames = vec_EnableInstanceLayer.data();
			} else {
				std::cerr << "EnableInstLayer is emtyp!\n";
				break;
			}

			///
			VkDebugUtilsMessengerCreateInfoEXT debugMessCIExte{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
				.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
				.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
				.pfnUserCallback = debugUtilsMessCallback,
			};
			debugMessCIExte.pNext = instanceCI.pNext;
			instanceCI.pNext = &debugMessCIExte;

			// 此处还可以补充 layerSettings

			///
			if (vkCreateInstance(&instanceCI, nullptr, &m_VkInstance) != VK_SUCCESS) {
				std::cerr << "vkCreateInstance failed!\n";
				break;
			}
		}

		if (!valid())
			break;

		rtn = true;
	} while (0);

	return rtn;
}

VkBool32
CVkManager::debugUtilsMessCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
								   VkDebugUtilsMessageTypeFlagsEXT messageType,
								   const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
								   void * pUserData) {
	return false; // do nothing
}

bool CVkManager::valid() {
	return (m_VkManagerInstance != nullptr) && (m_VkInstance != VK_NULL_HANDLE);
}

CVkManager::~CVkManager() {
	if (m_VkManagerInstance != nullptr)
		delete m_VkManagerInstance;

	// 还需要释放 VK 资源
}