#include "deviceManager.h"
#include <cassert>

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

		rtn = true;
	} while (0);

	return rtn;
}

bool CDeviceManager::valid() {
	return (m_DeviceManagerInstance != nullptr);
}

CDeviceManager::~CDeviceManager() {
	if (m_DeviceManagerInstance != nullptr)
		delete m_DeviceManagerInstance;

	// 还需要释放 VkDevice 的资源
}