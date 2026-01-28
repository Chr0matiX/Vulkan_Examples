#pragma once

#include "vulkan/vulkan_core.h"
#include "vulkanManager.h"
#include <cstdint>
#include <map>

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

		uint32_t m_DesiredNumberOfSwapchainImages{0};

		VkSurfaceCapabilitiesKHR m_SurfaceCaps;

		VkSurfaceTransformFlagsKHR m_PreTransform;

		VkCompositeAlphaFlagBitsKHR m_CompositeAlpha{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR};

		VkPresentModeKHR m_SwapchainPresentMode{VK_PRESENT_MODE_FIFO_KHR};

		std::map<uint32_t, std::vector<VkCommandBuffer>> map_Index2VecCmdBuffer;

	private:
		bool initManager();

		bool recreateSwapchain();
		bool beforeRcSwapchain();
		bool afterRcSwapchain();

		std::vector<VkCommandBuffer> getCommandBuffer(const uint32_t & queueIndex);

	public:
		bool valid();
		void destroyManager();

		static CSwapchainManager & getInstance();
};