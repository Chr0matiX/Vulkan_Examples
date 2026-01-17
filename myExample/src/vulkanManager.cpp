#include "vulkanManager.h"
#include "vulkan/vulkan_core.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

CVkMana * CVkMana::m_VkManaInst = nullptr;

CVkMana & CVkMana::getInst() {
	if (m_VkManaInst == nullptr) {
		m_VkManaInst = new CVkMana();
		m_VkManaInst->initMana();
	}

	return *m_VkManaInst;
}

bool CVkMana::initMana() {
	bool rtn = false;

	do {
		// Create VkInstace
		{
			VkInstanceCreateInfo instCI{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

			///
			VkApplicationInfo appInfo{.sType =
										  VK_STRUCTURE_TYPE_APPLICATION_INFO,
									  .pApplicationName = m_AppName,
									  .pEngineName = m_Engine,
									  .apiVersion = m_ApiVersion};
			instCI.pApplicationInfo = &appInfo;

			///
			std::vector<const char *> vec_EnableInstExte;
			{
				uint32_t exteCount = 0;
				vkEnumerateInstanceExtensionProperties(nullptr, &exteCount,
													   nullptr);
				if (exteCount <= 0) {
					std::cerr << "exteCount is zero!\n";
					break;
				}

				vec_EnableInstExte.reserve(exteCount);

				std::vector<VkExtensionProperties> vec_ExteProp{exteCount};
				if (vkEnumerateInstanceExtensionProperties(
						nullptr, &exteCount, &vec_ExteProp.front()) !=
					VK_SUCCESS) {
					std::cerr
						<< "EnumerateInstanceExtensionProperties failed!\n";
					break;
				}

				// for (const auto & extProp : vec_ExtProp)
				//	vec_EnableInstExte.emplace_back(extProp.extensionName);

				for (const auto & expectInstExte : vec_ExpectInstExte) {
					const auto & it_exteProp = std::find_if(
						vec_ExteProp.begin(), vec_ExteProp.end(),
						[&expectInstExte](
							const VkExtensionProperties & exteProp) -> bool {
							return strcmp(exteProp.extensionName,
										  expectInstExte) == 0;
						});

					if (it_exteProp != vec_ExteProp.end())
						vec_EnableInstExte.emplace_back(
							it_exteProp->extensionName);
				}
			}
			if (!vec_EnableInstExte.empty()) {
				instCI.enabledExtensionCount =
					static_cast<uint32_t>(vec_EnableInstExte.size());
				instCI.ppEnabledExtensionNames = vec_EnableInstExte.data();
			} else {
				std::cerr << "EnableInstExte is emtyp!\n";
				break;
			}

			///
			std::vector<const char *> vec_EnableInstLayer;
			{
				uint32_t layerCount = 0;
				vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
				if (layerCount <= 0) {
					std::cerr << "layerCount is zero!\n";
					break;
				}

				vec_EnableInstLayer.reserve(layerCount);

				std::vector<VkLayerProperties> vec_LayerProp{layerCount};
				if (vkEnumerateInstanceLayerProperties(
						&layerCount, vec_LayerProp.data()) != VK_SUCCESS) {
					std::cerr << "EnumerateInstanceLayerProperties failed!\n";
					break;
				}

				for (const auto & expectInstLayer : vec_ExpectInstLayer) {
					const auto & it_LayerProp = std::find_if(
						vec_LayerProp.begin(), vec_LayerProp.end(),
						[&expectInstLayer](
							const VkLayerProperties & layerProp) -> bool {
							return strcmp(expectInstLayer,
										  layerProp.layerName) == 0;
						});

					if (it_LayerProp != vec_LayerProp.end())
						vec_EnableInstLayer.emplace_back(
							it_LayerProp->layerName);
				}
			}
			if (!vec_EnableInstLayer.empty()) {
				instCI.enabledLayerCount =
					static_cast<uint32_t>(vec_EnableInstLayer.size());
				instCI.ppEnabledLayerNames = vec_EnableInstLayer.data();
			} else {
				std::cerr << "EnableInstLayer is emtyp!\n";
				break;
			}

			///
			VkDebugUtilsMessengerCreateInfoEXT debugMessCIExte{
				.sType =
					VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
				.messageSeverity =
					VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
					VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
				.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
				.pfnUserCallback = debugUtilsMessCallback,
			};
			debugMessCIExte.pNext = instCI.pNext;
			instCI.pNext = &debugMessCIExte;

			// 此处还可以补充 layerSettings

			///
			if (vkCreateInstance(&instCI, nullptr, &m_VkInst) != VK_SUCCESS) {
				std::cerr << "vkCreateInstance failed!\n";
				break;
			}
		}

		rtn = true;
	} while (0);

	return rtn;
}

VkBool32 CVkMana::debugUtilsMessCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
	void * pUserData) {
	return false; // do nothing
}