#pragma once

#include "WinCommon.h"

#include "Utils.hpp"

#include "vulkan/vulkan.h"

class SurfaceVulkan {
		SINGLETON_CLASS(SurfaceVulkan)
		friend class VkContext;

	private:
		/**********************************************************
		外部依赖
		**********************************************************/
		HINSTANCE m_AppInctance{nullptr};

		VkInstance m_VkInstance{VK_NULL_HANDLE};

		std::string m_MainWindowsClassName;

		std::string m_WindowsTitle;

		/**********************************************************
		资源
		**********************************************************/
		HWND m_WindowHandle{nullptr};

		VkSurfaceKHR m_SurfaceKHR{VK_NULL_HANDLE};

		int m_WindowWidth{1000};

		int m_WindowHeight{800};

	private:
		bool init();

		bool valid() const noexcept;

		void destroy();

		static LRESULT handleWindowMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		bool registerWindowClass();

		bool createWindow();

		bool isReady();
};