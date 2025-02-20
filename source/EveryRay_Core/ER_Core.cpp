#include <algorithm>

#include "ER_Core.h"
#include "ER_CoreException.h"
#include "ER_Utility.h"

#include "Common.h"

namespace EveryRay_Core
{
	static UINT appWidth = 0;
	static UINT appHeight = 0;
	DWORD dwStyle = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

	ER_Core::ER_Core(ER_RHI* aRHI, HINSTANCE instance, const std::wstring& windowClass, const std::wstring& windowTitle, int showCommand, bool isFullscreen)
		: 
		mInstance(instance),
		mWindowClass(windowClass),
		mWindowTitle(windowTitle),
		mShowCommand(showCommand),
		mWindowHandle(),
		mWindow(),
		mIsFullscreen(isFullscreen),
		mCoreClock(),
		mCoreTime(),
		mCoreComponents(),
		mCoreServices(),
		mCPUProfiler(new ER_CPUProfiler()),
		mRHI(aRHI)
	{
	}

	ER_Core::~ER_Core()
	{
	}

	HINSTANCE ER_Core::Instance() const
	{
		return mInstance;
	}

	HWND ER_Core::WindowHandle() const
	{
		return mWindowHandle;
	}

	const WNDCLASSEX& ER_Core::Window() const
	{
		return mWindow;
	}

	const std::wstring& ER_Core::WindowClass() const
	{
		return mWindowClass;
	}

	const std::wstring& ER_Core::WindowTitle() const
	{
		return mWindowTitle;
	}

	int ER_Core::ScreenWidth() const
	{
		return mScreenWidth;
	}

	int ER_Core::ScreenHeight() const
	{
		return mScreenHeight;
	}

	float ER_Core::AspectRatio() const
	{
		return static_cast<float>(mScreenWidth) / mScreenHeight;
	}

	void ER_Core::Run()
	{
		InitializeWindow();
		if (!mRHI->Initialize(mWindowHandle, mScreenWidth, mScreenHeight, mIsFullscreen))
			throw ER_CoreException("Could not initialize RHI or it is null!");
		else
			ER_OUTPUT_LOG(L"[ER Logger][ER_Core] Initialized graphics API. \n");
		Initialize();

		MSG message;
		ZeroMemory(&message, sizeof(message));

		mCoreClock.Reset();

		while (message.message != WM_QUIT)
		{
			if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
			else
			{
				mCoreClock.UpdateGameTime(mCoreTime);
				Update(mCoreTime);
				Draw(mCoreTime);

				mFrameIndex++;
			}
		}

		Shutdown();
	}

	void ER_Core::Exit()
	{
		PostQuitMessage(0);
	}

	void ER_Core::Shutdown()
	{
		DeleteObject(mCPUProfiler);
		DeleteObject(mRHI);
		UnregisterClass(mWindowClass.c_str(), mWindow.hInstance);
	}

	void ER_Core::Initialize()
	{
		for (ER_CoreComponent* component : mCoreComponents)
		{
			component->Initialize();
		}
		ER_OUTPUT_LOG(L"[ER Logger][ER_Core] Initialized core engine components. \n");
	}

	void ER_Core::Update(const ER_CoreTime& gameTime)
	{
		for (ER_CoreComponent* component : mCoreComponents)
		{
			if (component->Enabled())
			{
				component->Update(gameTime);
			}
		}
	}

	void ER_Core::WindowSizeChanged(int width, int height)
	{
		if (mRHI && width > 0 && height > 0)
		{
			mScreenHeight = height;
			mScreenWidth = width;

			mMainViewport.TopLeftX = 0.0f;
			mMainViewport.TopLeftY = 0.0f;
			mMainViewport.Width = static_cast<float>(width);
			mMainViewport.Height = static_cast<float>(height);
			mMainViewport.MinDepth = 0.0f;
			mMainViewport.MaxDepth = 1.0f;

			mMainRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };

			mRHI->OnWindowSizeChanged(width, height);
		}
	}

	void ER_Core::InitializeWindow()
	{
		ZeroMemory(&mWindow, sizeof(mWindow));

		mWindow.cbSize = sizeof(WNDCLASSEX);
		mWindow.style = CS_CLASSDC;
		mWindow.lpfnWndProc = WndProc;
		mWindow.hInstance = mInstance;
		mWindow.hIcon = (HICON)LoadImage(NULL, L"bunny.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
		mWindow.hIconSm = (HICON)LoadImage(NULL, L"bunny.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
		mWindow.hCursor = LoadCursor(nullptr, IDC_ARROW);
		mWindow.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		mWindow.lpszClassName = mWindowClass.c_str();

		RECT windowRectangle = { 0, 0, static_cast<LONG>(mScreenWidth), static_cast<LONG>(mScreenHeight) };
		AdjustWindowRect(&windowRectangle, dwStyle, FALSE);

		RegisterClassEx(&mWindow);
		POINT center = CenterWindow(mScreenWidth, mScreenHeight);
		mWindowHandle = CreateWindow(mWindowClass.c_str(), mWindowTitle.c_str(), dwStyle, center.x, center.y, windowRectangle.right - windowRectangle.left, windowRectangle.bottom - windowRectangle.top, nullptr, nullptr, mInstance, nullptr);

		ShowWindow(mWindowHandle, mShowCommand);
		UpdateWindow(mWindowHandle);

		SetWindowLongPtr(mWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		ER_OUTPUT_LOG(L"[ER Logger][ER_Core] Initialized window. \n");
	}

	LRESULT WINAPI ER_Core::WndProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		if (ImGui_ImplWin32_WndProcHandler(windowHandle, message, wParam, lParam))
			return true;

		static bool s_in_sizemove = false;
		static bool s_in_suspend = false;
		static bool s_minimized = false;
		static bool s_fullscreen = false;

		ER_Core* core = reinterpret_cast<ER_Core*>(GetWindowLongPtr(windowHandle, GWLP_USERDATA));

		// WARNING: we do not allow window resizing except going into fullscreen (which is also the same resolution, as in non-fullscreen)
		switch (message)
		{

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_SIZE:
			if (!s_in_sizemove && core)
			{
				//core->WindowSizeChanged(mScreenWidth, mScreenHeight); //this is very strict and only allows default resolution

				//uncomment for proper resizing
				//core->WindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
			}
			break;

		case WM_ENTERSIZEMOVE:
			s_in_sizemove = true;
			break;

		case WM_EXITSIZEMOVE:
			s_in_sizemove = false;
			break;
			//uncomment for proper resizing
			//if (core)
			//{
			//	RECT rc;
			//	GetClientRect(windowHandle, &rc);
			//	core->WindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
			//}
		//case WM_SYSKEYDOWN:
		//	if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
		//	{
		//		// Implements the classic ALT+ENTER fullscreen toggle
		//		if (s_fullscreen && appWidth > 0 && appHeight > 0)
		//		{
		//			SetWindowLongPtr(windowHandle, GWL_STYLE, dwStyle);
		//			SetWindowLongPtr(windowHandle, GWL_EXSTYLE, 0);
		//			ShowWindow(windowHandle, SW_SHOWNORMAL);
		//			SetWindowPos(windowHandle, HWND_TOP, 0, 0, appWidth, appHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
		//		}
		//		else
		//		{
		//			SetWindowLongPtr(windowHandle, GWL_STYLE, 0);
		//			SetWindowLongPtr(windowHandle, GWL_EXSTYLE, dwStyle);
		//			SetWindowPos(windowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		//			ShowWindow(windowHandle, SW_SHOWMAXIMIZED);
		//		}
		//
		//		s_fullscreen = !s_fullscreen;
		//	}
		//	break;
		}

		return DefWindowProc(windowHandle, message, wParam, lParam);
	}

	POINT ER_Core::CenterWindow(int windowWidth, int windowHeight)
	{
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		POINT center;
		center.x = (screenWidth - windowWidth) / 2;
		center.y = (screenHeight - windowHeight) / 2;

		return center;
	}
}