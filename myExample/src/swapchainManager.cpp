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
		// 同步机制
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

		VkPipelineCacheCreateInfo pipelineCacheCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
		if (vkCreatePipelineCache(CDeviceManager::getInstance().getLogicalDevice(),
								  &pipelineCacheCreateInfo, nullptr,
								  &m_PipelineCache) != VK_SUCCESS)
			return false;

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
	vkDeviceWaitIdle(CDeviceManager::getInstance().getLogicalDevice());

	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(CDeviceManager::getInstance().getLogicalDevice(), m_Pipeline, nullptr);
	}
	if (m_PipelineCache != VK_NULL_HANDLE) {
		vkDestroyPipelineCache(CDeviceManager::getInstance().getLogicalDevice(), m_PipelineCache,
							   nullptr);
	}

	for (auto & framebuffer : vec_FrameBuffer) {
		if (framebuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(CDeviceManager::getInstance().getLogicalDevice(), framebuffer,
								 nullptr);
		}
	}
	vec_FrameBuffer.clear();

	if (m_RenderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(CDeviceManager::getInstance().getLogicalDevice(), m_RenderPass,
							nullptr);
	}

	if (m_ImageViewDepthStencil != VK_NULL_HANDLE) {
		vkDestroyImageView(CDeviceManager::getInstance().getLogicalDevice(),
						   m_ImageViewDepthStencil, nullptr);
	}
	if (m_ImageDepthStencil != VK_NULL_HANDLE) {
		vkDestroyImage(CDeviceManager::getInstance().getLogicalDevice(), m_ImageDepthStencil,
					   nullptr);
	}
	if (m_MemoryDepthStencil != VK_NULL_HANDLE) {
		vkFreeMemory(CDeviceManager::getInstance().getLogicalDevice(), m_MemoryDepthStencil,
					 nullptr);
	}

	for (auto & imageView : vec_ImageView) {
		if (imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(CDeviceManager::getInstance().getLogicalDevice(), imageView,
							   nullptr);
		}
	}
	vec_ImageView.clear();
	vec_Image.clear();

	for (uint32_t i = 0; i < CVulkanManager::getInstance().getMaxConcurrentFrames(); ++i) {
		if (vec_PresentCplSmph[i] != VK_NULL_HANDLE)
			vkDestroySemaphore(CDeviceManager::getInstance().getLogicalDevice(),
							   vec_PresentCplSmph[i], nullptr);
		if (vec_RenderCplSmph[i] != VK_NULL_HANDLE)
			vkDestroySemaphore(CDeviceManager::getInstance().getLogicalDevice(),
							   vec_RenderCplSmph[i], nullptr);
		if (vec_waitFence[i] != VK_NULL_HANDLE)
			vkDestroyFence(CDeviceManager::getInstance().getLogicalDevice(), vec_waitFence[i],
						   nullptr);
	}

	if (m_SwapChain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(CDeviceManager::getInstance().getLogicalDevice(), m_SwapChain,
							  nullptr);
	}

	if (m_SwapchainManagerInstance != nullptr) {
		delete m_SwapchainManagerInstance;
		m_SwapchainManagerInstance = nullptr;
	}
}

bool CSwapchainManager::recreateSwapchain() {
	vkDeviceWaitIdle(CDeviceManager::getInstance().getLogicalDevice());

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
		vec_ImageView.clear();

		// VkImage 则由管理的 Swapchain 释放
		// 此处还有延迟释放的规则，若其中的资源处于被使用的状态，那么会在使用完毕之后释放
		vkDestroySwapchainKHR(CDeviceManager::getInstance().getLogicalDevice(), oldSwapchain,
							  nullptr);
	}

	return afterRcSwapchain();
}

bool CSwapchainManager::beforeRcSwapchain() {
	{
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
				CDeviceManager::getInstance().getPhysicalDevice(),
				CSurfaceManager::getInstance().getSurfaceKHR(), &m_SurfaceCaps) != VK_SUCCESS)
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
	}

	// 显示模式
	{
		uint32_t presentModeCount;
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(
				CDeviceManager::getInstance().getPhysicalDevice(),
				CSurfaceManager::getInstance().getSurfaceKHR(), &presentModeCount,
				nullptr) != VK_SUCCESS)
			return false;
		if (presentModeCount <= 0)
			return false;

		std::vector<VkPresentModeKHR> vec_PresentModes(presentModeCount);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(
				CDeviceManager::getInstance().getPhysicalDevice(),
				CSurfaceManager::getInstance().getSurfaceKHR(), &presentModeCount,
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
	}

	// m_SurfaceFormat
	{
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
	}

	return true;
}

bool CSwapchainManager::afterRcSwapchain() {
	// 清理资源
	cleanupSwapchainResources();

	// m_ImageViewDepthStencil
	{
		if (!setSupportedDepthFormat(false))
			return false;

		VkImageCreateInfo imageCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = m_DepthFormat,
			.extent = {m_SurfaceCaps.currentExtent.width, m_SurfaceCaps.currentExtent.height, 1},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT};

		if (vkCreateImage(CDeviceManager::getInstance().getLogicalDevice(), &imageCI, nullptr,
						  &m_ImageDepthStencil) != VK_SUCCESS)
			return false;

		VkMemoryRequirements memReqs{};
		vkGetImageMemoryRequirements(CDeviceManager::getInstance().getLogicalDevice(),
									 m_ImageDepthStencil, &memReqs);

		VkMemoryAllocateInfo memAllloc{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memReqs.size,
			.memoryTypeIndex = CDeviceManager::getInstance().getMemoryTypeIndex(
				memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

		if ((vkAllocateMemory(CDeviceManager::getInstance().getLogicalDevice(), &memAllloc, nullptr,
							  &m_MemoryDepthStencil) != VK_SUCCESS) ||
			(vkBindImageMemory(CDeviceManager::getInstance().getLogicalDevice(),
							   m_ImageDepthStencil, m_MemoryDepthStencil, 0) != VK_SUCCESS))
			return false;

		VkImageViewCreateInfo imageViewCI{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
										  .image = m_ImageDepthStencil,
										  .viewType = VK_IMAGE_VIEW_TYPE_2D,
										  .format = m_DepthFormat,
										  .subresourceRange = {
											  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
											  .baseMipLevel = 0,
											  .levelCount = 1,
											  .baseArrayLayer = 0,
											  .layerCount = 1,
										  }};

		if (m_DepthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
			imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		if (vkCreateImageView(CDeviceManager::getInstance().getLogicalDevice(), &imageViewCI,
							  nullptr, &m_ImageViewDepthStencil) != VK_SUCCESS)
			return false;
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

	if (vec_Image.empty() || (vec_Image.size() != vec_ImageView.size()))
		return false;

	if (m_RenderPass == VK_NULL_HANDLE)
		setRenderPass();

	vec_FrameBuffer.resize(m_ImageCount);
	for (uint32_t i = 0; i < m_ImageCount; ++i) {
		const VkImageView attachments[2] = {vec_ImageView[i], m_ImageViewDepthStencil};
		VkFramebufferCreateInfo frameBufferCI{.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
											  .renderPass = m_RenderPass,
											  .attachmentCount = 2,
											  .pAttachments = attachments,
											  .width = m_SurfaceCaps.currentExtent.width,
											  .height = m_SurfaceCaps.currentExtent.height,
											  .layers = 1};
		if (vkCreateFramebuffer(CDeviceManager::getInstance().getLogicalDevice(), &frameBufferCI,
								nullptr, &vec_FrameBuffer[i]) != VK_SUCCESS)
			continue;
	}
	if (vec_FrameBuffer.size() != m_ImageCount)
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

bool CSwapchainManager::setSupportedDepthFormat(const bool & requiresStencil) {
	const std::vector<VkFormat> & vec_Format =
		requiresStencil ? vec_DepthStencilFormat : vec_DepthFormat;

	if (vec_Format.empty())
		return false;

	VkFormatProperties formatProps;
	for (const auto & format : vec_Format) {
		vkGetPhysicalDeviceFormatProperties(CDeviceManager::getInstance().getPhysicalDevice(),
											format, &formatProps);
		if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			m_DepthFormat = format;
			return true;
		}
	}

	return false;
}

bool CSwapchainManager::setRenderPass() {
	// 定义不同的缓存规格，和操作定义
	std::vector<VkAttachmentDescription> attachments{
		// Color attachment
		VkAttachmentDescription{.format = m_SurfaceFormat.format,
								.samples = VK_SAMPLE_COUNT_1_BIT,
								.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
								.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
								.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
								.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
								.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
								.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR},
		// Depth attachment
		VkAttachmentDescription{.format = m_DepthFormat,
								.samples = VK_SAMPLE_COUNT_1_BIT,
								.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
								.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
								.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
								.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
								.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
								.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}};

	// 定义数组引用，attachment 表示下标
	VkAttachmentReference colorReference{.attachment = 0,
										 .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
	VkAttachmentReference depthReference{
		.attachment = 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

	VkSubpassDescription subpassDescription{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorReference,
		.pDepthStencilAttachment = &depthReference,
	};

	// Subpass dependencies for layout transitions
	std::vector<VkSubpassDependency> dependencies{
		VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
							VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
							VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
							 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
		},
		VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask =
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		}};

	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpassDescription,
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data(),
	};

	return (vkCreateRenderPass(CDeviceManager::getInstance().getLogicalDevice(), &renderPassInfo,
							   nullptr, &m_RenderPass) == VK_SUCCESS);
}

void CSwapchainManager::cleanupSwapchainResources() {
	VkDevice device = CDeviceManager::getInstance().getLogicalDevice();

	for (auto & fb : vec_FrameBuffer) {
		if (fb != VK_NULL_HANDLE)
			vkDestroyFramebuffer(device, fb, nullptr);
	}
	vec_FrameBuffer.clear();

	if (m_ImageViewDepthStencil != VK_NULL_HANDLE) {
		vkDestroyImageView(device, m_ImageViewDepthStencil, nullptr);
		m_ImageViewDepthStencil = VK_NULL_HANDLE;
	}
	if (m_ImageDepthStencil != VK_NULL_HANDLE) {
		vkDestroyImage(device, m_ImageDepthStencil, nullptr);
		m_ImageDepthStencil = VK_NULL_HANDLE;
	}
	if (m_MemoryDepthStencil != VK_NULL_HANDLE) {
		vkFreeMemory(device, m_MemoryDepthStencil, nullptr);
		m_MemoryDepthStencil = VK_NULL_HANDLE;
	}

	// for (auto & view : vec_ImageView) {
	//	if (view != VK_NULL_HANDLE)
	//		vkDestroyImageView(device, view, nullptr);
	// }
}