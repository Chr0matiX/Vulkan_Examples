#include "surfaceManager.h"
#include "vulkanManager.h"

#include <cassert>
#include <iostream>

CSurfaceManager * CSurfaceManager::m_SurfaceManagerInstance{nullptr};

CSurfaceManager & CSurfaceManager::getInstance() {
	if (m_SurfaceManagerInstance == nullptr) {
		m_SurfaceManagerInstance = new CSurfaceManager();
		assert(m_SurfaceManagerInstance->initManager());
	}

	return *m_SurfaceManagerInstance;
}

bool CSurfaceManager::initManager() {
	bool rtn = false;

	do {
		// HWND m_WindowHandle 句柄
		{
			WNDCLASSEX wndClass{
				.cbSize = sizeof(WNDCLASSEX),
				// 水平或垂直尺寸发生变化时，强制重绘整个窗口
				.style = CS_HREDRAW | CS_VREDRAW,
				// lpfnWndProc：一个函数指针，指定窗口过程函数，键盘/鼠标输入、窗口大小变更都会发送给这个函数
				.lpfnWndProc = handleWindowMessages,
				.cbClsExtra = 0,
				.cbWndExtra = 0,
				// 当前应用程序实例的句柄
				.hInstance = CVulkanManager::getInstance().getAppInstance(),
				.hIcon = LoadIcon(NULL, IDI_APPLICATION),
				.hCursor = LoadCursor(NULL, IDC_ARROW),
				// 指定背景画刷为黑色，避免窗口在部分刷新场景下的闪烁
				.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
				.lpszMenuName = NULL,
				.lpszClassName = m_MainWindowsClassName,
				.hIconSm = LoadIcon(NULL, IDI_WINLOGO),
			};

			if (!RegisterClassEx(&wndClass)) {
				std::cerr << "Could not register window class!\n";
				break;
			}

			// 此处可以处理全屏逻辑

			DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
			DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

			RECT windowRect{
				.left = 0L, .top = 0L, .right = m_WindowWidth, .bottom = m_WindowHeight};

			AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

			const auto & windowRectWidth = windowRect.right - windowRect.left;
			const auto & WindowRectHeight = windowRect.bottom - windowRect.top;

			m_WindowHandle = CreateWindowEx(dwExStyle, m_MainWindowsClassName, m_WindowsTitle,
											dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0,
											windowRectWidth, WindowRectHeight, NULL, NULL,
											CVulkanManager::getInstance().getAppInstance(), NULL);

			if (m_WindowHandle == nullptr) {
				std::cerr << "CreateWindowEx failed!\n";
				break;
			}

			// 设置窗口位置
			uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRectWidth) / 2;
			uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - WindowRectHeight) / 2;
			SetWindowPos(m_WindowHandle, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

			ShowWindow(m_WindowHandle, SW_SHOW);
			SetForegroundWindow(m_WindowHandle);
			SetFocus(m_WindowHandle);
		}

		// VkSurfaceKHR m_SurfaceKHR
		{
			VkWin32SurfaceCreateInfoKHR surfaceCI{
				.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
				.hinstance = CVulkanManager::getInstance().getAppInstance(),
				.hwnd = m_WindowHandle};

			if (vkCreateWin32SurfaceKHR(CVulkanManager::getInstance().getVkInstance(), &surfaceCI,
										nullptr, &m_SurfaceKHR) != VK_SUCCESS) {
				std::cerr << "vkCreateWin32SurfaceKHR failed!\n";
				break;
			}
		}

		if (!valid())
			break;

		rtn = true;
	} while (0);

	return rtn;
}

bool CSurfaceManager::valid() {
	return (m_WindowHandle != nullptr) && (m_SurfaceKHR != VK_NULL_HANDLE);
}

void CSurfaceManager::destroyManager() {
	if (m_SurfaceManagerInstance != nullptr)
		delete m_SurfaceManagerInstance;

	vkDestroySurfaceKHR(CVulkanManager::getInstance().getVkInstance(), m_SurfaceKHR, nullptr);
	DestroyWindow(m_WindowHandle);
	UnregisterClass(m_MainWindowsClassName, CVulkanManager::getInstance().getAppInstance());
}

LRESULT CSurfaceManager::handleWindowMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (CVulkanManager::getInstance().valid() == true) {
		/*
		switch (uMsg) {
		case WM_CLOSE:
			prepared = false;
			DestroyWindow(hWnd);
			PostQuitMessage(0);
			break;
		case WM_PAINT:
			ValidateRect(window, NULL);
			break;
		case WM_KEYDOWN:
			switch (wParam) {
			case KEY_P:
				paused = !paused;
				break;
			case KEY_F1:
				ui.visible = !ui.visible;
				break;
			case KEY_F2:
				if (camera.type == Camera::CameraType::lookat) {
					camera.type = Camera::CameraType::firstperson;
				} else {
					camera.type = Camera::CameraType::lookat;
				}
				break;
			case KEY_ESCAPE:
				PostQuitMessage(0);
				break;
			}

			if (camera.type == Camera::firstperson) {
				switch (wParam) {
				case KEY_W:
					camera.keys.up = true;
					break;
				case KEY_S:
					camera.keys.down = true;
					break;
				case KEY_A:
					camera.keys.left = true;
					break;
				case KEY_D:
					camera.keys.right = true;
					break;
				}
			}

			keyPressed((uint32_t)wParam);
			break;
		case WM_KEYUP:
			if (camera.type == Camera::firstperson) {
				switch (wParam) {
				case KEY_W:
					camera.keys.up = false;
					break;
				case KEY_S:
					camera.keys.down = false;
					break;
				case KEY_A:
					camera.keys.left = false;
					break;
				case KEY_D:
					camera.keys.right = false;
					break;
				}
			}
			break;
		case WM_LBUTTONDOWN:
			mouseState.position = glm::vec2((float)LOWORD(lParam),
		(float)HIWORD(lParam)); mouseState.buttons.left = true; break; case
		WM_RBUTTONDOWN: mouseState.position = glm::vec2((float)LOWORD(lParam),
		(float)HIWORD(lParam)); mouseState.buttons.right = true; break; case
		WM_MBUTTONDOWN: mouseState.position = glm::vec2((float)LOWORD(lParam),
		(float)HIWORD(lParam)); mouseState.buttons.middle = true; break; case
		WM_LBUTTONUP: mouseState.buttons.left = false; break; case WM_RBUTTONUP:
			mouseState.buttons.right = false;
			break;
		case WM_MBUTTONUP:
			mouseState.buttons.middle = false;
			break;
		case WM_MOUSEWHEEL: {
			short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			camera.translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f));
			break;
		}
		case WM_MOUSEMOVE: {
			handleMouseMove(LOWORD(lParam), HIWORD(lParam));
			break;
		}
		case WM_SIZE:
			if ((prepared) && (wParam != SIZE_MINIMIZED)) {
				if ((resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam ==
		SIZE_RESTORED))) { destWidth = LOWORD(lParam); destHeight =
		HIWORD(lParam); windowResize();
				}
			}
			break;
		case WM_GETMINMAXINFO: {
			LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;
			minMaxInfo->ptMinTrackSize.x = 64;
			minMaxInfo->ptMinTrackSize.y = 64;
			break;
		}
		case WM_ENTERSIZEMOVE:
			resizing = true;
			break;
		case WM_EXITSIZEMOVE:
			resizing = false;
			break;
		}

		OnHandleMessage(hWnd, uMsg, wParam, lParam);
		*/
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}