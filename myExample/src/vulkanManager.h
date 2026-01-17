#pragma once

#include "vulkan/vulkan.h"

#include <vector>

class CVkMana {
private:
	static CVkMana * m_VkManaInst;

	VkInstance m_VkInst = VK_NULL_HANDLE;

	const char* m_AppName = "MyExample";
	const char* m_Engine = "MyEngine";
	const uint32_t m_ApiVersion = VK_API_VERSION_1_0;

	const std::vector<const char *> vec_ExpectInstExte{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	};

	const std::vector<const char *> vec_ExpectInstLayer{
		"VK_LAYER_KHRONOS_validation",
	};

private:
	CVkMana() = default;
	CVkMana(const CVkMana &) = delete;
	CVkMana(CVkMana &&) = delete;
	CVkMana & operator=(const CVkMana &) = delete;
	CVkMana & operator=(CVkMana &&) = delete;

	static CVkMana & getInst();
	bool initMana();

	static VkBool32 debugUtilsMessCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
		void * pUserData);

public:
};
