#include "deviceManager.h"
#include "vulkan/vulkan_core.h"
#include "vulkanManager.h"

#include <cassert>
#include <cstdint>
#include <iostream>

CDeviceManager * CDeviceManager::m_DeviceManagerInstance{nullptr};

CDeviceManager & CDeviceManager::getInst() {
	if (m_DeviceManagerInstance == nullptr) {
		m_DeviceManagerInstance = new CDeviceManager();
		assert(m_DeviceManagerInstance->initManager());
	}

	return *m_DeviceManagerInstance;
}

bool CDeviceManager::initManager() {
	bool rtn = false;

	do {
		uint32_t gpuCount{0};
		vkEnumeratePhysicalDevices(CVulkanManager::getInstance().getVkInstance(), &gpuCount,
								   nullptr);
		if (gpuCount <= 0) {
			std::cerr << "gpuCount is zero!\n";
			break;
		}

		std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
		if (vkEnumeratePhysicalDevices(CVulkanManager::getInstance().getVkInstance(), &gpuCount,
									   nullptr) != VK_SUCCESS) {
			std::cerr << "vkEnumeratePhysicalDevices failed!\n";
			break;
		}

		// gpu 打分选取

		if (!valid())
			break;

		rtn = true;
	} while (0);

	return rtn;
}

bool CDeviceManager::valid() {
	return true;
}

CDeviceManager::~CDeviceManager() {
	if (m_DeviceManagerInstance != nullptr)
		delete m_DeviceManagerInstance;

	// 还需要释放 VkDevice 的资源
}