#pragma once

#include "vulkan/vulkan_core.h"
#include "vulkanManager.h"
#include <cstdarg>
#include <cstdint>
#include <optional>
#include <set>

class CDeviceManager {
		SINGLETON_CLASS(CDeviceManager)

	private:
		static CDeviceManager * m_DeviceManagerInstance;

		VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};

		VkDevice m_LogicalDevice{VK_NULL_HANDLE};

		VkPhysicalDeviceProperties m_Property;

		VkPhysicalDeviceFeatures m_Features;

		VkPhysicalDeviceMemoryProperties m_MemoryProperty;

		const VkQueueFlags m_QueueFlags{VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT |
										VK_QUEUE_COMPUTE_BIT};

		const std::vector<const char *> vec_ExpectDeviceExtension{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

		VkPhysicalDeviceFeatures m_ExpectDeviceFeatures{};

		static const float m_DefaultQueuePriority;

		struct {
			private:
				std::optional<uint32_t> m_Graphics;
				std::optional<uint32_t> m_Present;
				std::optional<uint32_t> m_Transfer;
				std::optional<uint32_t> m_Compute;

			public:
				inline bool valid() const {
					return m_Graphics.has_value() && m_Present.has_value();
				}
				inline bool complete() const {
					return m_Graphics.has_value() && m_Present.has_value() &&
						   m_Transfer.has_value() && m_Compute.has_value();
				}

				inline void setGraphics(const uint32_t & index) { m_Graphics = index; }
				inline void setPresent(const uint32_t & index) { m_Present = index; }
				inline void setTransfer(const uint32_t & index) { m_Transfer = index; }
				inline void setCompute(const uint32_t & index) { m_Compute = index; }

				inline uint32_t getGraphics() const { return m_Graphics.value(); }
				inline uint32_t getPresent() const { return m_Present.value(); }
				inline uint32_t getTransfer() const {
					return m_Transfer.value_or(m_Graphics.value());
				}
				inline uint32_t getCompute() const {
					return m_Compute.value_or(m_Graphics.value());
				}

				inline std::set<uint32_t> getQueueIndexes() {
					return std::set<uint32_t>{getGraphics(), getPresent(), getTransfer(),
											  getCompute()};
				}
		} m_QueueIndex;

	private:
		bool initManager();

		// 此处还需要为整体配置联动，这里写的太简陋了
		int ratePhysicalDevice(const VkPhysicalDevice & physicalDevice);

	public:
		bool valid();
		void destroyManager();

		static CDeviceManager & getInstance();
		inline VkPhysicalDevice getPhysicalDevice() { return m_PhysicalDevice; };
		inline VkDevice getLogicalDevice() { return m_LogicalDevice; };
};