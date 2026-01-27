#include "swapchainManager.h"
#include "deviceManager.h"
#include "surfaceManager.h"
#include "vulkan/vulkan_core.h"
#include "vulkanManager.h"
#include <cassert>
#include <cstdint>
#include <algorithm>

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
		uint32_t surfaceFormatCount;
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(CDeviceManager::getInstance().getPhysicalDevice(),
												 CSurfaceManager::getInstance().getSurfaceKHR(),
												 &surfaceFormatCount, NULL) != VK_SUCCESS)
			break;
		if (surfaceFormatCount <= 0)
			break;

		std::vector<VkSurfaceFormatKHR> vec_SurfaceFormats(surfaceFormatCount);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(CDeviceManager::getInstance().getPhysicalDevice(),
												 CSurfaceManager::getInstance().getSurfaceKHR(),
												 &surfaceFormatCount,
												 vec_SurfaceFormats.data()) != VK_SUCCESS)
			break;

		m_SurfaceFormat = vec_SurfaceFormats[0];
		for (auto & availableFormat : vec_SurfaceFormats) {
			if (std::find(vec_ExpectPreferredImageFormats.begin(),
						  vec_ExpectPreferredImageFormats.end(),
						  availableFormat.format) != vec_ExpectPreferredImageFormats.end()) {
				m_SurfaceFormat = availableFormat;
				break;
			}
		}

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

		// 不在这里调用，可能更符合逻辑
		//if (!recreateSwapchain())
		//	break;

		rtn = true;
	} while (0);

	return rtn;
}

bool CSwapchainManager::valid() {
	return (m_SwapchainManagerInstance != nullptr);
}

void CSwapchainManager::destroyManager() {
	if (m_SwapchainManagerInstance != nullptr)
		delete m_SwapchainManagerInstance;
}

bool CSwapchainManager::recreateSwapchain() {
	VkSwapchainKHR oldSwapchain = m_SwapChain;

	VkSurfaceCapabilitiesKHR surfaceCaps;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(CDeviceManager::getInstance().getPhysicalDevice(),
												  CSurfaceManager::getInstance().getSurfaceKHR(),
												  &surfaceCaps) != VK_SUCCESS)
		return false;

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
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (size_t i = 0; i < presentModeCount; i++) {
		// Mailbox：不撕裂，低延迟
		if (vec_PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			// 若找到，则直接使用 Mailbox
			break;
		}
		// Immediate：可能存在撕裂，低延迟
		if (vec_PresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}

	// 选择缓冲个数
	uint32_t desiredNumberOfSwapchainImages = surfaceCaps.minImageCount + 1;
	if ((surfaceCaps.maxImageCount > 0) &&
		(desiredNumberOfSwapchainImages > surfaceCaps.maxImageCount)) {
		desiredNumberOfSwapchainImages = surfaceCaps.maxImageCount;
	}

	// 处理旋转，目前没有相关需求
	VkSurfaceTransformFlagsKHR preTransform;
	if (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		// We prefer a non-rotated transform
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	} else {
		preTransform = surfaceCaps.currentTransform;
	}

	// 处理混合方式
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	for (auto & compositeAlphaFlag : vec_ExpectCompositeAlphaFlags) {
		if (surfaceCaps.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		};
	}

	VkSwapchainCreateInfoKHR swapchainCI{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = CSurfaceManager::getInstance().getSurfaceKHR(),
		.minImageCount = desiredNumberOfSwapchainImages,
		.imageFormat = m_SurfaceFormat.format,
		.imageColorSpace = m_SurfaceFormat.colorSpace,
		.imageExtent = {surfaceCaps.currentExtent.width, surfaceCaps.currentExtent.height},
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform,
		.compositeAlpha = compositeAlpha,
		.presentMode = swapchainPresentMode,
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
	if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// 可以作为传输目的，可以向内直接写入一张渲染好的位图
	if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
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

	return vec_Image.size() == vec_ImageView.size();
}