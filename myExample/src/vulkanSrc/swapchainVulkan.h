#pragma once

#include "../Utils.hpp"

#include "vulkan/vulkan.h"

#include <map>
#include <vector>

class SwapchainVulkan {
		SINGLETON_CLASS(SwapchainVulkan)
		friend class VkContext;

	private:
		/**********************************************************
		外部依赖
		**********************************************************/
		uint32_t m_MaxConcurrentFrames{2};

		VkDevice m_LogicalDevice{VK_NULL_HANDLE};

		VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};

		VkSurfaceKHR m_SurfaceKHR{VK_NULL_HANDLE};

		std::vector<VkCompositeAlphaFlagBitsKHR> vec_ExpectCompositeAlphaFlags;

		std::vector<VkFormat> vec_ExpectPreferredImageFormats;

		std::vector<VkFormat> vec_DepthFormat;

		std::vector<VkFormat> vec_DepthStencilFormat;

		VkPhysicalDeviceMemoryProperties m_MemoryProperty;

		/**********************************************************
		资源
		**********************************************************/
		VkSwapchainKHR m_SwapChain{VK_NULL_HANDLE};

		uint32_t m_ImageCount{0};

		std::vector<VkImage> vec_Image;

		std::vector<VkImageView> vec_ImageView;

		DepthStencilRes m_DepthStencilRes;

		VkSurfaceFormatKHR m_SurfaceFormat;

		uint32_t m_DesiredNumberOfSwapchainImages{0};

		VkSurfaceCapabilitiesKHR m_SurfaceCaps;

		VkSurfaceTransformFlagsKHR m_PreTransform;

		VkCompositeAlphaFlagBitsKHR m_CompositeAlpha{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR};

		VkPresentModeKHR m_SwapchainPresentMode{VK_PRESENT_MODE_FIFO_KHR};

		VkRenderPass m_RenderPass{VK_NULL_HANDLE};

		std::vector<VkFramebuffer> vec_FrameBuffer;

	private:
		bool init();

		bool valid();

		void destroy();

		bool recreateSwapchain();

		bool beforeRcSwapchain();

		bool afterRcSwapchain();

		std::vector<VkCommandBuffer> getCommandBuffer(const uint32_t & queueIndex);

		bool setSupportedDepthFormat(const bool & requiresStencil);

		bool setRenderPass();

		void cleanupSwapchainRes();

		bool isReady();
};