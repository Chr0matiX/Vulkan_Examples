# 研究

- 研究`VulkanExampleBase::setupDepthStencil`中申请显存的细节
- 研究`VulkanExampleBase::createSynchronizationPrimitives`中不同同步机制的细节
- 研究`VulkanExampleBase::setupRenderPass`中缓存定义的细节

# 实现

1. 实现 CSwapchainManager
	1. 主要成员
		1. ~~VkSwapchainKHR~~
		2. ~~std::vector<VkImage>~~
		3. ~~std::vector<VkImageView>~~
		4. ~~VkFormat~~
		5. ~~VkExtent2D~~
	2. 主要功能
		1. 属性协商
		2. 资源创建
		3. 状态维护与重建
		4. 同步与交互接口