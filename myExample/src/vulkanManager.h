#pragma once

#include "WinCommon.h"

#include "vulkan/vulkan.h"

#include <vector>

class CVkManager {
	private:
		static CVkManager * m_VkManagerInstance;

		VkInstance m_VkInstance{VK_NULL_HANDLE};

		const char * m_AppName{"MyExample"};
		const char * m_Engine{"MyEngine"};
		const uint32_t m_ApiVersion{VK_API_VERSION_1_0};

		const std::vector<const char *> vec_ExpectInstanceExtension{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		};

		const std::vector<const char *> vec_ExpectInstanceLayer{
			"VK_LAYER_KHRONOS_validation",
		};

		HINSTANCE m_AppInctance{nullptr};

	private:
		CVkManager() = default;
		~CVkManager();
		CVkManager(const CVkManager &) = delete;
		CVkManager(CVkManager &&) = delete;
		CVkManager & operator=(const CVkManager &) = delete;
		CVkManager & operator=(CVkManager &&) = delete;

		bool initManager();

		static VkBool32
		debugUtilsMessCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
							   VkDebugUtilsMessageTypeFlagsEXT messageType,
							   const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
							   void * pUserData);

	public:
		bool valid();

		static CVkManager & getInstance();

		inline HINSTANCE getAppInstance() const { return m_AppInctance; };
		inline VkInstance getVkInstance() const { return m_VkInstance; };
};
