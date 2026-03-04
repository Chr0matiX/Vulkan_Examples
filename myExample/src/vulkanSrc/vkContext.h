#pragma once

#include "../WinCommon.h"

#include "../Utils.hpp"

#include "deviceVulkan.h"
#include "renderVulkan.h"
#include "surfaceVulkan.h"
#include "swapchainVulkan.h"

#include "vulkan/vulkan.h"

#include <cstdint>
#include <vector>

class VkContext {
		SINGLETON_CLASS(VkContext)
		friend class SurfaceVulkan;
		friend class DeviceVulkan;
		friend class SwapchainVulkan;

	private:
		static VkContext * m_VkContextInstance;

		/**********************************************************
		全局配置
		**********************************************************/
		std::string m_AppName{"MyExample"};

		std::string m_Engine{"MyEngine"};

		const uint32_t m_ApiVersion{VK_API_VERSION_1_0};

		// 期望实例扩展
		const std::vector<const char *> vec_ExpectInstanceExtension{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		};

		// 期望层
		const std::vector<const char *> vec_ExpectInstanceLayer{
			"VK_LAYER_KHRONOS_validation",
		};

		// 最大缓冲个数
		const uint32_t m_MaxConcurrentFrames{2};

		const char * m_MainWindowsClassName{"mainWindowClass"};

		const char * m_WindowsTitle{"MainWindow"};

		const std::vector<VkCompositeAlphaFlagBitsKHR> vec_ExpectCompositeAlphaFlags = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,			// 不透明
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,	// 预乘 Alpha
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, // 普通 Alpha
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,			// 继承
		};

		const std::vector<VkFormat> vec_ExpectPreferredImageFormats = {
			VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_A8B8G8R8_UNORM_PACK32};

		VkPhysicalDeviceFeatures m_ExpectDeviceFeatures;

		const std::vector<const char *> vec_ExpectDeviceExtension{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};

		const float m_DefaultQueuePriority{0.f};

		const std::vector<VkFormat> vec_DepthFormat{
			VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM};

		const std::vector<VkFormat> vec_DepthStencilFormat{
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
		};

		int m_WindowWidth{1000};

		int m_WindowHeight{800};

		/**********************************************************
		资源
		**********************************************************/
		VkInstance m_VkInstance{VK_NULL_HANDLE};

		SurfaceVulkan * m_SurfaceVulkanInstance{nullptr};

		DeviceVulkan * m_DeviceVulkanInstance{nullptr};

		SwapchainVulkan * m_SwapchainVulkanInstance{nullptr};

		RenderVulkan * m_RenderVulkanInstance{nullptr};

		Camera m_Camera;

		std::vector<Vertex> m_VecVertex;
		std::vector<uint32_t> m_VecIndex;

	private:
		// 输入事件回调
		static VkBool32
		debugUtilsMessCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
							   VkDebugUtilsMessageTypeFlagsEXT messageType,
							   const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
							   void * pUserData);

		bool
		getEnableInstaceExtension(std::vector<const char *> & vec_EnableInstanceExtension) const;

		bool getEnableInstaceLayer(std::vector<const char *> & vec_EnableInstanceLayer) const;

		void setDebugUtils(VkInstanceCreateInfo & vkInstanceCI) const;

	public:
		bool init();

		bool valid();

		void destroy();

		static VkContext & getInstance();

		void startRenderLoop();

		void setVertex(const std::vector<Vertex> & vec_Vertex) { m_VecVertex = vec_Vertex; }
		void setIndex(const std::vector<uint32_t> & vec_Index) { m_VecIndex = vec_Index; }
};
