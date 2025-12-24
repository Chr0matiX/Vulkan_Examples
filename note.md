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

