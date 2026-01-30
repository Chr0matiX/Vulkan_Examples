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

# 其他
1. 项目字段总览

	我先列举一下我的项目中，具体的字段成员，我的思路是围绕着字段成员包装一些对外的操作接口，所以我主要展示我的字段成员：

	1. CVulkanManager
		1. VkInstance
		2. HINSTANCE
	2. CDeviceManager
		1. VkPhysicalDevice
		2. VkDevice
		3. VkPhysicalDeviceProperties
		4. VkPhysicalDeviceFeatures
		5. VkPhysicalDeviceMemoryProperties
		6. struct QueueIndex
		7. std::map<uint32_t, VkQueue> map_Index2VkQueue;
		8. std::map<uint32_t, VkCommandPool> map_Index2CommandPool;
	3. CSurfaceManager
		1. HWND
		2. VkSurfaceKHR
		3. int m_WindowWidth{0};
		4. int m_WindowHeight{0};
	4. CSwapchainManager
		1. VkSwapchainKHR
		2. std::vector<VkImage> vec_Image;
		3. std::vector<VkImageView> vec_ImageView;
		4. VkSurfaceFormatKHR
		5. std::vector<VkSemaphore> vec_PresentCplSmph
		6. std::vector<VkSemaphore> vec_RenderCplSmph
		7. std::vector<VkFence> vec_waitFence

	以上就是我的大致结构，其中有些没有写变量名，但是我想这不影响结构的表示。

2. 动态/静态资源划分

	整理一下，有哪些资源是动态的（程序运行期间可能需要重建），有哪些资源是静态的（程序运行期间不需要重建或不需要频繁重建）。

	先整理一下静态资源：
	1. CVulkanManager 相关
	2. CDeviceManager 相关
	3. CSurfaceManager 相关

	再整理一下动态资源：
	1. VkSwapchainKHR
	2. vec_Image
	3. vec_ImageView
	4. VkSurfaceCapabilitiesKHR

	首先第一个问题，上述总结是否正确？是否完整？

	第二个问题内容稍多。我是否可以将 VkSwapchainKHR 视为一个重要的中间成员。
	围绕着 VkSwapchainKHR ，在重新创建 VkSwapchainKHR 之前，我需要读取哪些可能已经被修改的数据？（比如 VkSurfaceCapabilitiesKHR等等？）
	在重新创建 VkSwapchainKHR 之后，我需要继续重建什么资源？（比如 VkImage等等？）

3. 我现在还缺少~~VkImage (Depth/Stencil): 深度缓冲~~， ~~VkRenderPass，~~ VkFramebuffer， VkPipeline， ~~VkCommandBuffer~~
4. 重新总结

	我们现在需要以各种 CreateInfo 为单位，对资源初始化的方式进行总结。
	要求：
	1. 严格要求列举顺序，体现出依赖关系
	2. 若是一些枚举配置或简单的变量配置，则不需要列举出来（如VkInstanceCreateInfo需要VkApplicationInfo，而VkApplicationInfo内都是简单配置，所以VkApplicationInfo不需要单独列举出来，而VkInstanceCreateInfo需要表明需要VkApplicationInfo）
	3. 有些CreateInfo需要其他CreateInfo的产物，如FrameBuffer需要renderPass，renderPass又是 VkRenderPassCreateInfo的产物，此时在列举时主要表明产物，后面括号中写上对应的CreateInfo，如 renderPass(VkRenderPassCreateInfo)
	4. 列举参考例子项目为triangel.cpp，范围为 从创建 VkInstance 开始，到创建完成 pipeline为止

	例子（不完善，仅供格式内容要求参考）：
	1. VkWin32SurfaceCreateInfoKHR
		1. VkInstance(VkInstanceCreateInfo)
	2. VkDeviceCreateInfo
		1. VkDeviceQueueCreateInfo
	3. VkRenderPassCreateInfo
      	1. VkAttachmentDescription
      	2. VkSubpassDescription
      	3. VkSubpassDependency
      	4. VkDevice(VkDeviceCreateInfo)
	4. VkFramebufferCreateInfo
      	1. VkRenderPass(VkRenderPassCreateInfo)
      	2. VkSurfaceCapabilitiesKHR
      	3. VkDevice(VkDeviceCreateInfo)

	类似于以上这种形式，理解我的意图，并为我整理。