#include "deviceVulkan.h"
#include "vulkan/vulkan_core.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>

bool DeviceVulkan::init() {
	bool rtn = false;

	do {
		VkDeviceCreateInfo deviceCI{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

		uint32_t gpuCount{0};
		vkEnumeratePhysicalDevices(m_VkInstance, &gpuCount, nullptr);
		if (gpuCount <= 0) {
			std::cerr << "gpuCount is zero!\n";
			break;
		}

		std::vector<VkPhysicalDevice> vec_PhysicalDevice(gpuCount);
		CHECK_VK_RESULT(
			vkEnumeratePhysicalDevices(m_VkInstance, &gpuCount, vec_PhysicalDevice.data()));

		// gpu 打分选取
		int highestScore{-1};
		size_t physicalDeviceIndex = 0;
		for (size_t index = 0; index < vec_PhysicalDevice.size(); ++index) {
			const auto & score = ratePhysicalDevice(vec_PhysicalDevice[index]);
			if (score > highestScore) {
				highestScore = score;
				physicalDeviceIndex = index;
			}
		}

		if (highestScore < 0) {
			std::cerr << "Pick GPU failed!\n";
			break;
		}

		m_PhysicalDevice = vec_PhysicalDevice[physicalDeviceIndex];
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_Property);
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_Features);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperty);

		std::vector<VkDeviceQueueCreateInfo> vec_QueueCI;
		std::set<uint32_t> set_QueueIndexes;
		{
			uint32_t queueFamilyCount{0};
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
			if (queueFamilyCount <= 0)
				break;

			std::vector<VkQueueFamilyProperties> vec_queueFamilyProperty(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount,
													 vec_queueFamilyProperty.data());

			int graphicsQSMax{0}, transferQSMax{0}, computeQSMax{0}, graphicsQS{0}, transferQS{0},
				computeQS{0};
			bool hasPresentQ{false};

			for (uint32_t queuePropertyIndex = 0;
				 queuePropertyIndex < vec_queueFamilyProperty.size(); ++queuePropertyIndex) {
				const auto & queueProperty = vec_queueFamilyProperty[queuePropertyIndex];
				const auto & queueFlags = queueProperty.queueFlags;

				VkBool32 presentSupport{0};
				if ((queueFlags & VK_QUEUE_GRAPHICS_BIT) || !hasPresentQ)
					vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, queuePropertyIndex,
														 m_SurfaceKHR, &presentSupport);

				if (queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					graphicsQS = 1;

					if (presentSupport)
						graphicsQS += 10;

					if (graphicsQS > graphicsQSMax) {
						graphicsQSMax = graphicsQS;
						m_QueueIndex.setGraphics(queuePropertyIndex);

						if (presentSupport) {
							hasPresentQ = true;
							m_QueueIndex.setPresent(queuePropertyIndex);
						}
					}
				}

				if (!hasPresentQ && presentSupport) {
					hasPresentQ = true;
					m_QueueIndex.setPresent(queuePropertyIndex);
				}

				if (queueFlags & VK_QUEUE_TRANSFER_BIT) {
					transferQS = 1;

					if (!(queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))) {
						transferQS += 10;
					}

					transferQS += queueProperty.queueCount;

					if (transferQS > transferQSMax) {
						transferQSMax = transferQS;
						m_QueueIndex.setTransfer(queuePropertyIndex);
					}
				}

				if (queueFlags & VK_QUEUE_COMPUTE_BIT) {
					computeQS = 1;

					if (!(queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
						computeQS += 10;
					}

					computeQS += queueProperty.queueCount;

					if (computeQS > computeQSMax) {
						computeQSMax = computeQS;
						m_QueueIndex.setCompute(queuePropertyIndex);
					}
				}
			}

			if (!m_QueueIndex.valid())
				break;

			set_QueueIndexes = m_QueueIndex.getQueueIndexes();
			const auto & queueIndexesCount = set_QueueIndexes.size();
			if (queueIndexesCount <= 0)
				break;

			vec_QueueCI.reserve(queueIndexesCount);

			for (const auto & queueIndex : set_QueueIndexes) {
				VkDeviceQueueCreateInfo queueCI{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = queueIndex,
					.queueCount = 1,
					.pQueuePriorities = &m_DefaultQueuePriority,
				};
				vec_QueueCI.emplace_back(queueCI);
			}

			if (vec_QueueCI.empty())
				break;
		}
		deviceCI.queueCreateInfoCount = vec_QueueCI.size();
		deviceCI.pQueueCreateInfos = vec_QueueCI.data();

		std::vector<const char *> vec_EnableDeviceExtension;
		{
			uint32_t extensionCount{0};
			vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount,
												 nullptr);
			if (extensionCount <= 0)
				break;

			vec_EnableDeviceExtension.reserve(extensionCount);

			std::vector<VkExtensionProperties> vec_DeviceExtensionProperty{extensionCount};
			CHECK_VK_RESULT(vkEnumerateDeviceExtensionProperties(
				m_PhysicalDevice, nullptr, &extensionCount, vec_DeviceExtensionProperty.data()));

			for (const auto & expectDeviceExtension : vec_ExpectDeviceExtension) {
				if (std::find_if(
						vec_DeviceExtensionProperty.begin(), vec_DeviceExtensionProperty.end(),
						[&expectDeviceExtension](
							const VkExtensionProperties & deviceExtensionProperty) -> bool {
							return strcmp(expectDeviceExtension,
										  deviceExtensionProperty.extensionName) == 0;
						}) != vec_DeviceExtensionProperty.end())
					vec_EnableDeviceExtension.emplace_back(expectDeviceExtension);
			}

			if (vec_EnableDeviceExtension.empty())
				break;
		}
		deviceCI.enabledExtensionCount = vec_EnableDeviceExtension.size();
		deviceCI.ppEnabledExtensionNames = vec_EnableDeviceExtension.data();

		// Features 选取还需要补充
		deviceCI.pEnabledFeatures = &m_ExpectDeviceFeatures;

		CHECK_VK_RESULT(vkCreateDevice(m_PhysicalDevice, &deviceCI, nullptr, &m_LogicalDevice));

		VkQueue vkQueue;
		VkCommandPool vkCommandPool;
		VkCommandPoolCreateInfo commandPoolCI{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		};

		if (!valid())
			break;

		rtn = true;
	} while (0);

	return rtn;
}

bool DeviceVulkan::valid() {
	return (m_PhysicalDevice != VK_NULL_HANDLE) && (m_LogicalDevice != VK_NULL_HANDLE) &&
		   m_QueueIndex.valid();
}

void DeviceVulkan::destroy() {
	if (m_LogicalDevice != VK_NULL_HANDLE) {
		map_QIndex2Queue.clear();

		vkDestroyDevice(m_LogicalDevice, nullptr);
		m_LogicalDevice = VK_NULL_HANDLE;
	}
}

int DeviceVulkan::ratePhysicalDevice(const VkPhysicalDevice & physicalDevice) {

	// if (!checkDeviceExtensionSupport(physicalDevice))
	//	return -1;

	// 队列族支持
	{
		uint32_t queueFamilyCount{0};
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		if (queueFamilyCount <= 0)
			return -1;

		std::vector<VkQueueFamilyProperties> vec_queueFamilyProperty(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
												 vec_queueFamilyProperty.data());

		bool graphicsFamily{false}, presentFamily{false};
		// for (const auto & queueFamilyProperty : vec_queueFamilyProperty) {
		for (uint32_t queuePropertyIndex = 0; queuePropertyIndex < vec_queueFamilyProperty.size();
			 ++queuePropertyIndex) {
			const auto & queueFamilyProperty = vec_queueFamilyProperty[queuePropertyIndex];
			if (!graphicsFamily && (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT))
				graphicsFamily = true;

			if (!presentFamily) {
				VkBool32 presentSupport{0};
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queuePropertyIndex,
													 m_SurfaceKHR, &presentSupport);
				if (presentSupport)
					presentFamily = true;
			}

			if (graphicsFamily && presentFamily)
				break;
		}
		if (!graphicsFamily || !presentFamily)
			return -1;
	}

	{
		uint32_t extensionCount{0};
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		if (extensionCount <= 0)
			return -1;

		std::vector<VkExtensionProperties> vec_AvailableDeviceExtensionsProperty(extensionCount);
		if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount,
												 vec_AvailableDeviceExtensionsProperty.data()) !=
			VK_SUCCESS)
			return -1;

		for (const auto & expectDeviceExtension : vec_ExpectDeviceExtension) {
			const auto & it_ExtensionProperty = std::find_if(
				vec_AvailableDeviceExtensionsProperty.begin(),
				vec_AvailableDeviceExtensionsProperty.end(),
				[&expectDeviceExtension](const VkExtensionProperties & extensionProperty) -> bool {
					return strcmp(extensionProperty.extensionName, expectDeviceExtension) == 0;
				});

			if (it_ExtensionProperty == vec_AvailableDeviceExtensionsProperty.end())
				return -1;
		}
	}

	int score{0};

	VkPhysicalDeviceProperties physicalDeviceProperty;
	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperty;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperty);
	vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperty);

	//
	switch (physicalDeviceProperty.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		score += 1000;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		score += 500;
		break;
	default:
		break;
	}

	//
	for (uint32_t memIndex = 0; memIndex < physicalDeviceMemoryProperty.memoryHeapCount;
		 ++memIndex) {
		const auto & memoryHeap = physicalDeviceMemoryProperty.memoryHeaps[memIndex];
		if (memoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			score += static_cast<int32_t>(memoryHeap.size / (1024 * 1024 * 1024) * 100);
	}

	//
	if (physicalDeviceFeatures.samplerAnisotropy)
		score += 100;

	return score;
}

VkQueue DeviceVulkan::getVkQueue(const uint32_t & queueIndex) {
	if (!map_QIndex2Queue.contains(queueIndex)) {
		VkQueue vkQueue;
		vkGetDeviceQueue(m_LogicalDevice, queueIndex, 0, &vkQueue);
		map_QIndex2Queue[queueIndex] = vkQueue;
		return vkQueue;
	}

	return map_QIndex2Queue.at(queueIndex);
}