# Vulkan 初始化标准流程

## 1. 建立驱动连接 (The Instance)

* **加载 Loader**：链接库文件。
* **查询与设置 (Layers & Extensions)**：
    * **Layers**：如验证层（Validation Layers）。
    * **Instance Extensions**：**必须**包含 `VK_KHR_surface` 和平台特定扩展（如 `VK_KHR_win32_surface`）。
* **创建 `VkInstance`**：整个应用的根句柄。
* **设置 Debug Messenger**：尽早关联回调，监控后续所有初始化错误。

## 2. 窗口与显示表面 (The Surface)

> **Note**：在正式筛选硬件前，通常先创建 Surface，因为它是筛选 GPU 的条件之一。
* **创建 `VkSurfaceKHR`**：将 OS 的窗口句柄（HWND）封装给 Vulkan。

## 3. 硬件物理设备处理 (Physical Device)

* **获取 `VkPhysicalDevice`**：枚举所有可用 GPU。
* **查询队列族 (Queue Families)**：遍历并记录支持不同功能的索引：
    * **Graphics Index**：支持图形指令的队列。
    * **Present Index**：**必须**检查该队列族是否支持刚刚创建的 `Surface`。
    * **Compute/Transfer Index**：（可选）用于异步计算或数据传输。
* **查询能力与特性**：`Properties`（性能限制）、`Features`（可选功能开关）、`MemoryProperties`（显存堆分布）。

## 4. 逻辑设备与队列获取 (Logical Device & Queues)

* **创建 `VkDevice`**：
    * **启用特性 (Features)**：勾选具体的硬件功能（如 `samplerAnisotropy`）。
    * **设置设备扩展 (Device Extensions)**：**必须**包含 `VK_KHR_swapchain`。
    * **定义队列创建信息**：告诉 Vulkan 你要从哪些队列族里取多少条“传送带”。
* **获取 `VkQueue` 句柄**：通过 `vkGetDeviceQueue` 拿到真正的队列操作句柄。

## 5. 交换链配置 (The Swapchain)

* **创建 `VkSwapchainKHR`**：
    * **协商参数**：
        * **图像数量**：如三缓冲（Triple Buffering）。
        * **图像格式**：`VkFormat`（如 B8G8R8A8_SRGB）。
        * **呈现模式**：`VkPresentModeKHR`（如 `MAILBOX` 或 `FIFO`）。
        * **变换与合成**：如是否旋转 90 度，是否开启 Alpha 混合。
* **资源获取**：
    * **获取 `VkImage`**：从交换链拿到那些由驱动管理的图片句柄。
    * **创建 `VkImageView`**：为每张 Image 穿上“马甲”，描述它的用途（如作为 Render Target）。

## 6. 资源基础设施 (Infrastructure)

* **命令池 (VkCommandPool)**：分配命令缓冲区的内存池，通常与 `QueueFamilyIndex` 绑定。
* **同步原语 (Synchronization)**：
    * **`VkSemaphore`**：用于 GPU 内部不同任务间的同步（如：等图传完了再开始画）。
    * **`VkFence`**：用于 CPU 和 GPU 间的同步（如：等 GPU 画完了，CPU 再录制下一帧）。

# 交换链的本质
> **Note**：一个由显示驱动程序（OS/GPU）托管的“图像环形缓冲区（Image Ring Buffer）”及其所有权管理协议。

## 1. 物理本质

交换链是实实在在的显存空间，这些空间就是`VkImage`。

普通的显存只有 GPU 能看见，但是交换链管理的这几块显存很特殊。他们是 GPU 和 **显示器（窗口管理器）** 的“公共租界”，只有在交换链中的图像，操作系统才能扫描并显示在物理设备上。

## 2. 逻辑本质

交换链是一个**状态机**和**队列**，交换链中的每一个图像都拥有一个**状态**：
* 空闲（Available）
* 绘制中（Acquired/Rendering）
* 排队中（Queued for Present）
* 显示中（Presented/Scanning）

## 3. 协议本质

交换链本质上是在协调三个步调完全不同的设备：
* CPU：录制指令
* GPU：执行渲染
* 显示器：固定频率刷新

交换链的作用就是 **“缓冲/同步”**，协调这几个设备的运行节奏。

# 渲染依赖整理（倒置）
render()：

1. vkQueueSubmit
	- <- VkQueue(外部依赖)
	- <- VkSubmitInfo
		- <- VkCommandBuffer
		- <- VkSemaphore(present)
		- <- VkSemaphore(render)
	- <- VkFence

2. VkCommandBuffer(std::vector<VkCommandBuffer> 中获取)
	- <- vkBeginCommandBuffer <- VkCommandBufferBeginInfo
	- <- vkCmdBeginRenderPass <- VkRenderPassBeginInfo
		- <- VkRenderPass(外部依赖)
		- <- VkClearValue
		- <- **VkFramebuffer**(vector<VkFramebuffer> 中获取)(TODO：这里frameBuffer为什么是一个数组？其中成员的关联是什么，比如说其中每个元素应该与什么有联系？与VkImage有联系？)
	- <- vkCmdSetViewport <- VkViewport
	- <- vkCmdSetScissor <- VkRect2D
	- <- vkCmdBindDescriptorSets
		- <- **VkPipelineLayout**
		- <- **VkDescriptorSet**
	- <- vkCmdBindPipeline <- **VkPipeline**
	- <- vkCmdBindVertexBuffers <- **VkBuffer(vertices)**
	- <- vkCmdBindIndexBuffer <- **VkBuffer(indices)**
	- <- vkCmdDrawIndexed <- indexCount
	- <- vkCmdEndRenderPass
	- <- vkEndCommandBuffer

# 依赖关系整理
1. VkInstanceCreateInfo
	- <- InstanceExtensions(实例扩展)
	- <- VkApplicationInfo
	- <- layer(层级)
	- -> VkInstance
2. VkPhysicalDevice
	- -> VkPhysicalDeviceProperties
	- -> VkPhysicalDeviceFeatures
	- -> VkPhysicalDeviceMemoryProperties
	- -> VkFormat(DepthFormat)
3. VkDeviceQueueCreateInfo
	- <- queueIndex <- VkQueueFlagBits
4. VkDeviceCreateInfo
	- <- VkDeviceQueueCreateInfo
	- <- DeviceExtensions(设备扩展)
	- <- VkPhysicalDeviceFeatures
	- -> VkDevice
5. VkCommandPoolCreateInfo
	- <- VkCommandPoolCreateFlags
	- <- queueIndex
	- <- VkDevice
	- -> VkCommandPool
6. CreateWindowEx
	- <- RECT(画布大小)
	- <- HINSTANCE(应用实例)
	- -> HWND
7. VkWin32SurfaceCreateInfoKHR
	- <- HINSTANCE
	- <- HWND
	- -> VkSurfaceKHR
8. vkGetPhysicalDeviceSurfaceFormatsKHR
	- <- VkPhysicalDevice
	- <- VkSurfaceKHR
	- -> VkSurfaceFormatKHR
		- -> VkFormat(SurfaceFormat)
		- -> VkColorSpaceKHR
9. VkCommandPoolCreateInfo
	- <- queueIndex
	- -> VkCommandPool
10. vkGetPhysicalDeviceSurfaceCapabilitiesKHR
	- <- VkPhysicalDevice
	- <- VkSurfaceKHR
	- -> VkSurfaceCapabilitiesKHR
		- -> currentExtent
			- -> width
			- -> height
		- -> minImageCount(缓冲个数)
		- -> currentTransform(旋转)
		- -> supportedCompositeAlpha(混合方式)
11. vkGetPhysicalDeviceSurfacePresentModesKHR
	- <- VkPhysicalDevice
	- <- VkSurfaceKHR
	- -> VkPresentModeKHR(垂直同步)
12. VkSwapchainCreateInfoKHR
	- <- VkSurfaceKHR
	- <- VkSurfaceCapabilitiesKHR
		- <- minImageCount
		- <- currentExtent
		- <- currentTransform
		- <- supportedCompositeAlpha
	- <- VkSurfaceFormatKHR
		- <- VkFormat
		- <- VkColorSpaceKHR
	- <- VkPresentModeKHR
	- <- oldSwapchain
	- <- VkDevice
	- -> VkSwapchainKHR
13. vkGetSwapchainImagesKHR
	- <- VkDevice
	- <- VkSwapchainKHR
	- -> VkImage
14. vkCreateImageView
	- <- VkImageViewCreateInfo
		- <- VkImage
		- <- VkFormat(SurfaceFormat)
	- -> VkImageView
15. VkCommandBufferAllocateInfo
	- <- VkCommandPool
	- <- VkDevice
	- -> VkCommandBuffer
16. VkFenceCreateInfo/VkSemaphoreCreateInfo
	- <- device
	- -> VkFence/VkSemaphore
17. VkImageCreateInfo
	- <- VkFormat(DepthFormat)
	- <- currentExtent
	- <- VkDevice
	- -> VkImage(DepthStencil)
18. vkGetImageMemoryRequirements
    - <- VkDevice
    - <- VkImage(DepthStencil)
    - -> VkMemoryRequirements
19. VkMemoryAllocateInfo
	- <- VkMemoryRequirements
	- <- memIndex <- VkPhysicalDeviceMemoryProperties
	- <- VkDevice
	- -> VkDeviceMemory(DepthStencil)
20. vkBindImageMemory
	- <- VkDevice
	- <- VkImage(DepthStencil)
	- <- VkDeviceMemory(DepthStencil)
21. VkRenderPassCreateInfo
	- <- VkAttachmentDescription
		- <- VkFormat(SurfaceFormat)
		- <- VkFormat(DepthFormat)
	- <- VkSubpassDescription
		- <- VkAttachmentReference(SurfaceAttachmentIndex)
		- <- VkAttachmentReference(DepthAttachmentIndex)
	- <- VkSubpassDependency
	- <- VkDevice
	- -> VkRenderPass
22. VkPipelineCacheCreateInfo
	- <- VkDevice
	- -> VkPipelineCache
23. VkFramebufferCreateInfo
	- <- VkImageView_Array
		- <- VkImageView(SurfaceAttachment)
		- <- VkImageView(DepthAttachment)
	- <- VkDevice
	- -> VkFramebuffer
24. VkBufferCreateInfo
	- <- BufferSize
	- <- Usage
	- <- VkDevice
	- -> VkBuffer
25. VkMemoryAllocateInfo
	- <- VkMemoryRequirements <- VkBuffer
	- <- VkDevice
	- -> VkDeviceMemory
26. VkCommandBufferAllocateInfo
	- <- commandPool
	- <- VkDevice
	- -> VkCommandBuffer
27. VkCommandBuffer(copyCmd)
	- <- VkCommandBufferBeginInfo
	- <- VkBufferCopy
		- <- VkCommandBuffer
		- <- BufferSize
		- <- VkBuffer(Src)
		- <- VkBuffer(Dst)
	- <- vkEndCommandBuffer
28. vkQueueSubmit
	- <- VkSubmitInfo
		- <- VkCommandBuffer
	- <- VkFence <- VkFenceCreateInfo
	- <- VkQueue
29. vkWaitForFences
	- <- VkDevice
30. createUniformBuffers
	- -> VkDeviceMemory
	- -> VkBuffer
    	- -> ShaderData
    		- -> projectionMatrix
    		- -> modelMatrix
    		- -> viewMatrix
31. VkDescriptorSetLayoutCreateInfo
	- <- VkDescriptorSetLayoutBinding
	- <- VkDevice
	- -> VkDescriptorSetLayout
32. VkDescriptorPoolCreateInfo
	- <- VkDescriptorPoolSize
	- <- VkDevice
	- -> VkDescriptorPool
33. VkDescriptorSetAllocateInfo
	- <- VkDescriptorPool
	- <- VkDescriptorSetLayout
	- <- VkDevice
	- -> VkDescriptorSet
34. VkWriteDescriptorSet
	- <- VkDescriptorBufferInfo <- ShaderData
	- <- VkDescriptorSet
	- <- VkDevice
	- -> vkUpdateDescriptorSets
35. VkPipelineLayoutCreateInfo
	- <- VkDescriptorSetLayout
	- <- VkDevice
	- -> VkPipelineLayout
36. VkGraphicsPipelineCreateInfo
    - <- VkRenderPass
    - <- VkPipelineLayout
    - <- VkPipelineInputAssemblyStateCreateInfo
    - <- VkPipelineRasterizationStateCreateInfo
    - <- VkPipelineColorBlendStateCreateInfo
		- <- VkPipelineColorBlendAttachmentState
	- <- VkPipelineViewportStateCreateInfo
	- <- VkPipelineDynamicStateCreateInfo
		- <- VkDynamicState
	- <- VkPipelineDepthStencilStateCreateInfo
	- <- VkPipelineMultisampleStateCreateInfo
	- <- VkPipelineVertexInputStateCreateInfo
		- <- VkVertexInputBindingDescription
		- <- VkVertexInputAttributeDescription
	- <- VkPipelineShaderStageCreateInfo
	- <- VkDevice
	- -> VkPipeline

## AI输出内容

### 第一部分：初始化流程 (Initialization)

#### 1. 实例与设备层 (Instance & Device)

1. **VkInstanceCreateInfo**
* `<-` VkApplicationInfo (应用名称/版本)
* `<-` InstanceExtensions (扩展：Surface, Win32Surface, DebugUtils)
* `<-` LayerNames (验证层：StandardValidation)
* `->` **VkInstance**


2. **VkPhysicalDevice** (由 Instance 枚举选择)
* `->` VkPhysicalDeviceProperties (设备属性：名称, 类型, Limits)
* `->` VkPhysicalDeviceFeatures (设备特性：GeometryShader, MultiViewport 等)
* `->` VkPhysicalDeviceMemoryProperties (内存堆与类型)
* `->` VkFormatProperties (检查 Tiling 特性和 Format 支持)
* `->` **queueFamilyIndex** (找到支持 Graphics 和 Present 的队列族)


3. **VkDeviceCreateInfo** (逻辑设备)
* `<-` VkDeviceQueueCreateInfo `<-` queueFamilyIndex
* `<-` VkPhysicalDeviceFeatures (启用的特性)
* `<-` DeviceExtensions (设备扩展：Swapchain)
* `->` **VkDevice**
* `->` **VkQueue** (获取设备队列句柄)


4. **VkCommandPoolCreateInfo**
* `<-` queueFamilyIndex
* `<-` VkCommandPoolCreateFlags (ResetCommandBuffer)
* `<-` VkDevice
* `->` **VkCommandPool**



#### 2. 表面与交换链 (Surface & Swapchain)

5. **Window Surface**
* `<-` CreateWindowEx `->` HWND, HINSTANCE
* `<-` VkWin32SurfaceCreateInfoKHR `<-` HWND, HINSTANCE
* `->` **VkSurfaceKHR**


6. **Swapchain Support Queries** (协商阶段)
* `vkGetPhysicalDeviceSurfaceCapabilitiesKHR` `->` **VkSurfaceCapabilitiesKHR** (min/maxImageCount, currentExtent, transform)
* `vkGetPhysicalDeviceSurfaceFormatsKHR` `->` **VkSurfaceFormatKHR** (ColorSpace, Format)
* `vkGetPhysicalDeviceSurfacePresentModesKHR` `->` **VkPresentModeKHR** (Mailbox/Fifo)


7. **VkSwapchainCreateInfoKHR**
* `<-` VkSurfaceKHR
* `<-` minImageCount, imageExtent, preTransform (来自 Capabilities)
* `<-` imageFormat, imageColorSpace (来自 SurfaceFormat)
* `<-` imageUsage (ColorAttachment)
* `<-` VkPresentModeKHR
* `<-` oldSwapchain (用于窗口缩放重建)
* `<-` VkDevice
* `->` **VkSwapchainKHR**
* `->` `vkGetSwapchainImagesKHR` `->` **VkImage[]** (Swapchain Images)


8. **VkImageViewCreateInfo** (为 Swapchain Images 创建视图)
* `<-` VkImage (Swapchain Image [i])
* `<-` VkFormat (SurfaceFormat)
* `<-` VkComponentMapping
* `<-` VkImageSubresourceRange (Aspect: Color)
* `->` **VkImageView[]** (Swapchain ImageViews)



#### 3. 深度缓冲区资源 (Depth Buffer Resources)

9. **VkImageCreateInfo** (深度图)
* `<-` VkFormat (DepthFormat, e.g., D24_S8)
* `<-` VkExtent3D (Width, Height)
* `<-` VkImageUsageFlags (DepthStencilAttachment)
* `<-` VkDevice
* `->` **VkImage** (Depth Image)


10. **Memory Allocation (Depth)**
* `vkGetImageMemoryRequirements` `<-` VkImage (Depth) `->` VkMemoryRequirements
* `VkMemoryAllocateInfo` `<-` MemoryRequirements
* `VkMemoryAllocateInfo` `<-` memoryTypeIndex (DeviceLocal)
* `->` **VkDeviceMemory** (Depth Memory)
* `vkBindImageMemory` `<-` VkImage, VkDeviceMemory


11. **VkImageViewCreateInfo** (深度视图) **[补充]**
* `<-` VkImage (Depth Image)
* `<-` VkFormat (DepthFormat)
* `<-` VkImageSubresourceRange (Aspect: Depth | Stencil)
* `->` **VkImageView** (Depth ImageView)



#### 4. 渲染流程与帧缓冲 (Pass & Framebuffers)

12. **VkRenderPassCreateInfo**
* `<-` VkAttachmentDescription[0] (Color: LoadOp=Clear, StoreOp=Store, FinalLayout=PresentSrc)
* `<-` VkAttachmentDescription[1] (Depth: LoadOp=Clear, StoreOp=DontCare, FinalLayout=DepthStencilAtt)
* `<-` VkSubpassDescription (引用 AttachmentRef)
* `<-` VkSubpassDependency (外部依赖同步)
* `->` **VkRenderPass**


13. **VkFramebufferCreateInfo** (创建 N 个，对应 Swapchain Image 数量)
* `<-` VkRenderPass (兼容性句柄)
* `<-` VkImageView_Array
* `<-` VkImageView (Swapchain Color Attachment [i])
* `<-` VkImageView (Depth Attachment)


* `<-` Width, Height
* `->` **VkFramebuffer[]**



#### 5. 几何数据缓冲 (Vertex & Index Buffers)

14. **Staging Buffer Flow** (暂存缓冲流程 - 通常做法)
* `VkBufferCreateInfo` (Usage: TransferSrc) `->` StagingBuffer
* `vkMapMemory` `->` `memcpy` (CPU data) `->` `vkUnmapMemory`
* `VkBufferCreateInfo` (Usage: TransferDst | VertexBuffer) `->` **VkBuffer** (GPU VertexBuffer)
* `VkCommandBufferAllocateInfo` `->` TempCommandBuffer
* `vkBeginCommandBuffer`
* `vkCmdCopyBuffer` `<-` StagingBuffer, VertexBuffer


* `vkEndCommandBuffer` `->` `vkQueueSubmit` `->` `vkQueueWaitIdle`



#### 6. 描述符与 Uniform (Descriptors & Uniforms)

15. **Uniform Buffers** (多帧缓冲)
* `VkBufferCreateInfo` (Usage: UniformBuffer) `->` **VkBuffer[]**
* `VkMemoryAllocateInfo` (HostVisible | HostCoherent) `->` **VkDeviceMemory[]**
* `vkMapMemory` `->` **mapped pointer[]** (持久化映射)


16. **VkDescriptorSetLayoutCreateInfo** (定义布局)
* `<-` VkDescriptorSetLayoutBinding (Binding 0: UBO, VertexStage)
* `->` **VkDescriptorSetLayout**


17. **VkDescriptorPoolCreateInfo**
* `<-` PoolSize (Type: UBO, Count: MaxFrames)
* `<-` MaxSets
* `->` **VkDescriptorPool**


18. **VkDescriptorSetAllocateInfo**
* `<-` VkDescriptorPool
* `<-` VkDescriptorSetLayout
* `->` **VkDescriptorSet[]**


19. **VkWriteDescriptorSet** (更新绑定)
* `<-` VkDescriptorSet[i]
* `<-` VkDescriptorBufferInfo `<-` VkBuffer (Uniform [i])
* `->` `vkUpdateDescriptorSets`



#### 7. 渲染管线 (Graphics Pipeline)

20. **Shader Modules** **[补充]**
* `vkCreateShaderModule` `<-` SPIR-V Code (Vert) `->` **VkShaderModule** (Vert)
* `vkCreateShaderModule` `<-` SPIR-V Code (Frag) `->` **VkShaderModule** (Frag)


21. **VkPipelineLayoutCreateInfo**
* `<-` VkDescriptorSetLayout
* `->` **VkPipelineLayout**


22. **VkGraphicsPipelineCreateInfo**
* `<-` VkPipelineShaderStageCreateInfo (Vert/Frag Modules + EntryPoint)
* `<-` VkPipelineVertexInputStateCreateInfo (Binding/Attribute Description)
* `<-` VkPipelineInputAssemblyStateCreateInfo (Topology: TriangleList)
* `<-` VkPipelineViewportStateCreateInfo (Viewport/Scissor)
* `<-` VkPipelineRasterizationStateCreateInfo (CullMode, PolygonMode)
* `<-` VkPipelineMultisampleStateCreateInfo (MSAA)
* `<-` VkPipelineDepthStencilStateCreateInfo (DepthTest Enable)
* `<-` VkPipelineColorBlendStateCreateInfo (Blend Enable)
* `<-` VkPipelineDynamicStateCreateInfo (Dynamic: Viewport, Scissor)
* `<-` **VkPipelineLayout**
* `<-` **VkRenderPass**
* `->` **VkPipeline**



#### 8. 同步对象 (Sync Objects)

23. **Sync Setup**
* `VkSemaphoreCreateInfo` `->` **presentCompleteSemaphores[]**
* `VkSemaphoreCreateInfo` `->` **renderCompleteSemaphores[]**
* `VkFenceCreateInfo` (Flags: Signaled) `->` **waitFences[]**



---

### 第二部分：渲染循环流程 (Render Loop)

此部分对应 `render()` 函数，展示每一帧的动态依赖。

1. **CPU 帧同步 (CPU Sync)**
* **vkWaitForFences**
* `<-` **waitFences[currentFrame]** (阻断 CPU，直到 GPU 用完上一轮的这一帧资源)


* **vkResetFences**
* `<-` **waitFences[currentFrame]** (手动重置 Fence 为“未触发”，为本轮提交做准备)




2. **获取画布 (Acquire Image)**
* **vkAcquireNextImageKHR**
* `<-` VkSwapchainKHR
* `<-` **presentCompleteSemaphores[currentFrame]** (信标 1：填入此信号量，GPU 拿到图后会 Signal 它)
* `->` **imageIndex** (获取到的 Swapchain 图像索引，假设为 2)




3. **数据更新 (Update Data)**
* **Data Calc** `->` ShaderData Struct (CPU 栈上数据)
* **memcpy**
* `<-` ShaderData
* `->` **uniformBuffers[currentFrame].mapped** (写入 HostCoherent 内存，GPU 可直接读取)




4. **命令录制 (Command Recording)**
* **vkResetCommandBuffer**
* `<-` **commandBuffers[currentFrame]** (清空指令)


* **vkBeginCommandBuffer**
* `<-` VkCommandBufferBeginInfo (OneTimeSubmit)


* **Render Pass Scope**
* **vkCmdBeginRenderPass**
* `<-` **VkRenderPass** (定义流程)
* `<-` **frameBuffers[imageIndex]** (定义目标：**注意这里用的是 imageIndex，不是 currentFrame**)
* `<-` ClearValues (定义如何清屏)


* **Dynamic States**
* `vkCmdSetViewport` / `vkCmdSetScissor`


* **Binding**
* `vkCmdBindPipeline` `<-` **VkPipeline**
* `vkCmdBindDescriptorSets` `<-` **VkPipelineLayout**, **descriptorSets[currentFrame]**
* `vkCmdBindVertexBuffers` `<-` vertexBuffer
* `vkCmdBindIndexBuffer` `<-` indexBuffer


* **Draw**
* `vkCmdDrawIndexed` `<-` indexCount


* **vkCmdEndRenderPass**


* **vkEndCommandBuffer** `->` **Executable CommandBuffer**


5. **队列提交 (Queue Submit)**
* **VkSubmitInfo**
* `<-` **pWaitSemaphores = presentCompleteSemaphores[currentFrame]** (等待信标 1：等有图了再执行颜色输出)
* `<-` pWaitDstStageMask = COLOR_ATTACHMENT_OUTPUT
* `<-` **pCommandBuffers = commandBuffers[currentFrame]** (提交刚才录制的指令)
* `<-` **pSignalSemaphores = renderCompleteSemaphores[imageIndex]** (信标 2：画完后 Signal 它)


* **vkQueueSubmit**
* `<-` VkQueue
* `<-` **waitFences[currentFrame]** (操作完成后，Signal 这个 Fence，解开下一轮 CPU 的等待)




6. **画面呈现 (Present)**
* **VkPresentInfoKHR**
* `<-` **pWaitSemaphores = renderCompleteSemaphores[imageIndex]** (等待信标 2：等画完了再显示)
* `<-` VkSwapchainKHR
* `<-` **imageIndex** (呈现这一张)


* **vkQueuePresentKHR**
* `<-` VkQueue




7. **索引轮转 (Tick)**
* `currentFrame = (currentFrame + 1) % MAX_CONCURRENT_FRAMES`