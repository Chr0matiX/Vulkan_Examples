#pragma once

#include "vulkanManager.h"

class CDeviceManager {
	private:
		static CDeviceManager * m_DeviceManagerInstance;

	private:
		CDeviceManager() = default;
		~CDeviceManager();
		CDeviceManager(const CDeviceManager &) = delete;
		CDeviceManager(CDeviceManager &&) = delete;
		CDeviceManager & operator=(const CDeviceManager &) = delete;
		CDeviceManager & operator=(CDeviceManager &&) = delete;

		static CDeviceManager & getInst();
		bool initManager();

	public:
		bool valid();
};