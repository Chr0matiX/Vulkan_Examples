#pragma once

#include "vulkanManager.h"

class CSurfaceManager {
		SINGLETON_CLASS(CSurfaceManager)

	private:
		static CSurfaceManager * m_SurfaceManagerInstance;

		HWND m_WindowHandle{nullptr};

		VkSurfaceKHR m_SurfaceKHR{VK_NULL_HANDLE};

		const char * m_MainWindowsClassName = "mainWindowClass";
		const char * m_WindowsTitle = "MainWindow";

		int m_WindowWidth{0};
		int m_WindowHeight{0};

	private:
		bool initManager();

		static LRESULT handleWindowMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	public:
		bool valid();
		void destroyManager();

		static CSurfaceManager & getInstance();
		inline VkSurfaceKHR getSurfaceKHR() const noexcept { return m_SurfaceKHR; }
};