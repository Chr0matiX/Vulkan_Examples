#pragma once

#include "vulkan/vulkan_core.h"
#include "vulkanManager.h"

class CSwapchainManager {
		SINGLETON_CLASS(CSwapchainManager)

	private:
		static CSwapchainManager * m_SwapchainManagerInstance;

		VkSwapchainKHR m_SwapChain{VK_NULL_HANDLE};

		uint32_t m_ImageCount{0};
		std::vector<VkImage> vec_Image;
		std::vector<VkImageView> vec_ImageView;

		static const std::vector<VkCompositeAlphaFlagBitsKHR> vec_ExpectCompositeAlphaFlags;

		static const std::vector<VkFormat> vec_ExpectPreferredImageFormats;

		VkSurfaceFormatKHR m_SurfaceFormat;

		std::vector<VkSemaphore> vec_PresentCplSmph{
			CVulkanManager::getInstance().getMaxConcurrentFrames()};
		std::vector<VkSemaphore> vec_RenderCplSmph{
			CVulkanManager::getInstance().getMaxConcurrentFrames()};
		std::vector<VkFence> vec_waitFence{CVulkanManager::getInstance().getMaxConcurrentFrames()};

	private:
		bool initManager();

		bool recreateSwapchain();

	public:
		bool valid();
		void destroyManager();

		static CSwapchainManager & getInstance();
};