#pragma once

#include "../Utils.hpp"

#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <vector>

class DeviceVulkan {
		SINGLETON_CLASS(DeviceVulkan)
		friend class VkContext;

	private:
		/**********************************************************
		外部依赖
		**********************************************************/
		VkInstance m_VkInstance{VK_NULL_HANDLE};

		VkSurfaceKHR m_SurfaceKHR{VK_NULL_HANDLE};

		std::vector<const char *> vec_ExpectDeviceExtension;

		VkPhysicalDeviceFeatures m_ExpectDeviceFeatures{
			.multiDrawIndirect = true,
		};

		float m_DefaultQueuePriority{0.f};

		/**********************************************************
		资源
		**********************************************************/
		VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};

		VkDevice m_LogicalDevice{VK_NULL_HANDLE};

		VkPhysicalDeviceProperties m_Property;

		VkPhysicalDeviceFeatures m_Features;

		VkPhysicalDeviceMemoryProperties m_MemoryProperty;

		std::map<uint32_t, VkQueue> map_QIndex2Queue;

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

				inline std::set<uint32_t> getQueueIndexes() const {
					return std::set<uint32_t>{getGraphics(), getPresent(), getTransfer(),
											  getCompute()};
				}
		} m_QueueIndex;

	private:
		bool init();

		bool valid();

		void destroy();

		// 此处还需要为整体配置联动，这里写的太简陋了
		int ratePhysicalDevice(const VkPhysicalDevice & physicalDevice);

		inline std::set<uint32_t> getQueueIndexes() const { return m_QueueIndex.getQueueIndexes(); }

		VkQueue getVkQueue(const uint32_t & queueIndex);

		bool isReady();
};