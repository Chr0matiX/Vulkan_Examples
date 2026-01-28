#include "swapchainManager.h"
#include "deviceManager.h"
#include "surfaceManager.h"
#include "vulkan/vulkan_core.h"
#include "vulkanManager.h"
#include <algorithm>
#include <cassert>
#include <cstdint>

CSwapchainManager * CSwapchainManager::m_SwapchainManagerInstance{nullptr};

const std::vector<VkCompositeAlphaFlagBitsKHR> CSwapchainManager::vec_ExpectCompositeAlphaFlags = {
	VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,			// 不透明
	VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,	// 预乘 Alpha
	VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, // 普通 Alpha
	VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,			// 继承
};

const std::vector<VkFormat> CSwapchainManager::vec_ExpectPreferredImageFormats = {
	VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_A8B8G8R8_UNORM_PACK32};

CSwapchainManager & CSwapchainManager::getInstance() {
	if (m_SwapchainManagerInstance == nullptr) {
		m_SwapchainManagerInstance = new CSwapchainManager();
		assert(m_SwapchainManagerInstance->initManager());
	}

	return *m_SwapchainManagerInstance;
}

bool CSwapchainManager::initManager() {
	bool rtn = false;

	do {

		VkFenceCreateInfo fenceCI{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
								  .flags = VK_FENCE_CREATE_SIGNALED_BIT};
		VkSemaphoreCreateInfo semaphoreCI{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		for (uint32_t i = 0; i < CVulkanManager::getInstance().getMaxConcurrentFrames(); ++i) {
			if (vkCreateFence(CDeviceManager::getInstance().getLogicalDevice(), &fenceCI, nullptr,
							  &vec_waitFence[i]) != VK_SUCCESS)
				continue;

			if (vkCreateSemaphore(CDeviceManager::getInstance().getLogicalDevice(), &semaphoreCI,
								  nullptr, &vec_PresentCplSmph[i]) != VK_SUCCESS)
				continue;

			if (vkCreateSemaphore(CDeviceManager::getInstance().getLogicalDevice(), &semaphoreCI,
								  nullptr, &vec_RenderCplSmph[i]) != VK_SUCCESS)
				continue;
		}

		// 必须要初次创建一次
		if (!recreateSwapchain())
			break;

		rtn = true;
	} while (0);

	return rtn;
}

bool CSwapchainManager::valid() {
	return (m_SwapchainManagerInstance != nullptr) && (m_SwapChain != VK_NULL_HANDLE) &&
		   !vec_Image.empty() && (vec_Image.size() == vec_ImageView.size());
}

void CSwapchainManager::destroyManager() {
	if (m_SwapchainManagerInstance != nullptr)
		delete m_SwapchainManagerInstance;
}

bool CSwapchainManager::recreateSwapchain() {
	if (!beforeRcSwapchain())
		return false;

	VkSwapchainKHR oldSwapchain = m_SwapChain;

	VkSwapchainCreateInfoKHR swapchainCI{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = CSurfaceManager::getInstance().getSurfaceKHR(),
		.minImageCount = m_DesiredNumberOfSwapchainImages,
		.imageFormat = m_SurfaceFormat.format,
		.imageColorSpace = m_SurfaceFormat.colorSpace,
		.imageExtent = {m_SurfaceCaps.currentExtent.width, m_SurfaceCaps.currentExtent.height},
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.preTransform = (VkSurfaceTransformFlagBitsKHR)m_PreTransform,
		.compositeAlpha = m_CompositeAlpha,
		.presentMode = m_SwapchainPresentMode,
		// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the
		// surface area
		.clipped = VK_TRUE,
		// Setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse
		// and makes sure that we can still present already acquired images 旧交换链只做
		// 信息参考，在创建完成的一刻，已经没有作用了，所以，下面释放不会有任何问题
		.oldSwapchain = oldSwapchain,
	};

	// 可以看作开启复制粘贴
	// 可以作为传输源，可通过 vkCmdCopyImage 复制
	if (m_SurfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// 可以作为传输目的，可以向内直接写入一张渲染好的位图
	if (m_SurfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if (vkCreateSwapchainKHR(CDeviceManager::getInstance().getLogicalDevice(), &swapchainCI,
							 nullptr, &m_SwapChain) != VK_SUCCESS)
		return false;

	// 创建完成后立即尝试回收
	if (oldSwapchain != VK_NULL_HANDLE) {
		for (auto i = 0; i < vec_ImageView.size(); i++) {
			// 需要手动释放 VkImageView
			vkDestroyImageView(CDeviceManager::getInstance().getLogicalDevice(), vec_ImageView[i],
							   nullptr);
		}
		// VkImage 则由管理的 Swapchain 释放
		// 此处还有延迟释放的规则，若其中的资源处于被使用的状态，那么会在使用完毕之后释放
		vkDestroySwapchainKHR(CDeviceManager::getInstance().getLogicalDevice(), oldSwapchain,
							  nullptr);
	}

	return afterRcSwapchain();
}

bool CSwapchainManager::beforeRcSwapchain() {
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(CDeviceManager::getInstance().getPhysicalDevice(),
												  CSurfaceManager::getInstance().getSurfaceKHR(),
												  &m_SurfaceCaps) != VK_SUCCESS)
		return false;

	// 选择缓冲个数
	m_DesiredNumberOfSwapchainImages = m_SurfaceCaps.minImageCount + 1;
	if ((m_SurfaceCaps.maxImageCount > 0) &&
		(m_DesiredNumberOfSwapchainImages > m_SurfaceCaps.maxImageCount)) {
		m_DesiredNumberOfSwapchainImages = m_SurfaceCaps.maxImageCount;
	}

	// 处理旋转，目前没有相关需求
	if (m_SurfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		// We prefer a non-rotated transform
		m_PreTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	} else {
		m_PreTransform = m_SurfaceCaps.currentTransform;
	}

	// 处理混合方式
	for (auto & compositeAlphaFlag : vec_ExpectCompositeAlphaFlags) {
		if (m_SurfaceCaps.supportedCompositeAlpha & compositeAlphaFlag) {
			m_CompositeAlpha = compositeAlphaFlag;
			break;
		};
	}

	uint32_t presentModeCount;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(CDeviceManager::getInstance().getPhysicalDevice(),
												  CSurfaceManager::getInstance().getSurfaceKHR(),
												  &presentModeCount, nullptr) == VK_SUCCESS)
		return false;
	if (presentModeCount <= 0)
		return false;

	std::vector<VkPresentModeKHR> vec_PresentModes(presentModeCount);
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(CDeviceManager::getInstance().getPhysicalDevice(),
												  CSurfaceManager::getInstance().getSurfaceKHR(),
												  &presentModeCount,
												  vec_PresentModes.data()) != VK_SUCCESS)
		return false;

	// 默认垂直同步
	for (size_t i = 0; i < presentModeCount; i++) {
		// Mailbox：不撕裂，低延迟
		if (vec_PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			m_SwapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			// 若找到，则直接使用 Mailbox
			break;
		}
		// Immediate：可能存在撕裂，低延迟
		if (vec_PresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			m_SwapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}

	uint32_t surfaceFormatCount;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(CDeviceManager::getInstance().getPhysicalDevice(),
											 CSurfaceManager::getInstance().getSurfaceKHR(),
											 &surfaceFormatCount, NULL) != VK_SUCCESS)
		return false;
	if (surfaceFormatCount <= 0)
		return false;

	std::vector<VkSurfaceFormatKHR> vec_SurfaceFormats(surfaceFormatCount);
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(CDeviceManager::getInstance().getPhysicalDevice(),
											 CSurfaceManager::getInstance().getSurfaceKHR(),
											 &surfaceFormatCount,
											 vec_SurfaceFormats.data()) != VK_SUCCESS)
		return false;

	m_SurfaceFormat = vec_SurfaceFormats[0];
	for (auto & availableFormat : vec_SurfaceFormats) {
		if (std::find(vec_ExpectPreferredImageFormats.begin(),
					  vec_ExpectPreferredImageFormats.end(),
					  availableFormat.format) != vec_ExpectPreferredImageFormats.end()) {
			m_SurfaceFormat = availableFormat;
			break;
		}
	}

	return true;
}

bool CSwapchainManager::afterRcSwapchain() {
	if (vkGetSwapchainImagesKHR(CDeviceManager::getInstance().getLogicalDevice(), m_SwapChain,
								&m_ImageCount, nullptr) != VK_SUCCESS)
		return false;
	vec_Image.resize(m_ImageCount);
	if (vkGetSwapchainImagesKHR(CDeviceManager::getInstance().getLogicalDevice(), m_SwapChain,
								&m_ImageCount, vec_Image.data()) != VK_SUCCESS)
		return false;

	vec_ImageView.resize(m_ImageCount);

	for (auto i = 0; i < vec_Image.size(); i++) {
		VkImageViewCreateInfo colorAttachmentView{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = vec_Image[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = m_SurfaceFormat.format,
			.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
						   VK_COMPONENT_SWIZZLE_A},
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
								 .baseMipLevel = 0,
								 .levelCount = 1,
								 .baseArrayLayer = 0,
								 .layerCount = 1},
		};

		assert(vkCreateImageView(CDeviceManager::getInstance().getLogicalDevice(),
								 &colorAttachmentView, nullptr, &vec_ImageView[i]) == VK_SUCCESS);
	}

	if (vec_Image.empty() || (vec_Image.size() != vec_ImageView.size()))
		return false;

	return true;
}

std::vector<VkCommandBuffer> CSwapchainManager::getCommandBuffer(const uint32_t & queueIndex) {
	if (!map_Index2VecCmdBuffer.contains(queueIndex)) {
		std::vector<VkCommandBuffer> vec_CommandBuffer{
			CVulkanManager::getInstance().getMaxConcurrentFrames()};

		VkCommandBufferAllocateInfo cmdBufAllocateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = CDeviceManager::getInstance().getVkCommandPool(queueIndex),
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = static_cast<uint32_t>(vec_CommandBuffer.size()),
		};
		if (vkAllocateCommandBuffers(CDeviceManager::getInstance().getLogicalDevice(),
									 &cmdBufAllocateInfo, vec_CommandBuffer.data()) != VK_SUCCESS)
			return {};

		map_Index2VecCmdBuffer[queueIndex] = vec_CommandBuffer;
	}

	return map_Index2VecCmdBuffer[queueIndex];
}