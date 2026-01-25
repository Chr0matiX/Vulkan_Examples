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

	private:
		bool initManager();

		bool recreateSwapchain();

	public:
		bool valid();
		void destroyManager();

		static CSwapchainManager & getInstance();
};

/*
vkCreateSwapchainKHR 似乎需要在每一次更新的时候处理，需要单独提出来
std::vector<VkImage> 通过 swapchain 和数量直接创建出来
std::vector<VkImageView> 通过VkImage获取
VkFormat 通过 swapchain 获取，然后在通过配置查询，查询到第一个满足的可返回
可能还需要一个 colorSpace
VkExtent2D 可做局部，也可以做全局，表示显示大小
*/