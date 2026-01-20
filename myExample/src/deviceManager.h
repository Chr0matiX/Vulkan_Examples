#pragma once

#include "vulkanManager.h"

class CDeviceManager {
	private:
		static CDeviceManager * m_DeviceManagerInstance;

	private:
		CDeviceManager() = default;
		CDeviceManager(const CDeviceManager &) = delete;
		CDeviceManager(CDeviceManager &&) = delete;
		CDeviceManager & operator=(const CDeviceManager &) = delete;
		CDeviceManager & operator=(CDeviceManager &&) = delete;

		~CDeviceManager();

		static CDeviceManager & getInst();

	public:
		bool valid();

		bool initManager();
};