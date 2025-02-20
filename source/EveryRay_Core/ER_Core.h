#pragma once

#include "Common.h"
#include "ER_CoreClock.h"
#include "ER_CoreTime.h"
#include "ER_CoreComponent.h"
#include "ER_CoreServicesContainer.h"
#include "ER_Sandbox.h"
#include "ER_CPUProfiler.h"
#include "ER_Model.h"

#include "RHI\ER_RHI.h"

namespace EveryRay_Core
{
	class ER_Core
	{
	public:
		ER_Core(ER_RHI* aRHI, HINSTANCE instance, const std::wstring& windowClass, const std::wstring& windowTitle, int showCommand, bool isFullscreen);
		virtual ~ER_Core();

		HINSTANCE Instance() const;
		HWND WindowHandle() const;
		const WNDCLASSEX& Window() const;
		const std::wstring& WindowClass() const;
		const std::wstring& WindowTitle() const;
		int ScreenWidth() const;
		int ScreenHeight() const;
		float AspectRatio() const;
		void WindowSizeChanged(int width, int height);

		virtual void Run();
		virtual void Exit();
		virtual void Initialize();
		virtual void Update(const ER_CoreTime& gameTime);
		virtual void Draw(const ER_CoreTime& gameTime) = 0;

		virtual ER_Model* AddOrGet3DModelFromCache(const std::string& aFullPath, bool* didExist = nullptr, bool isSilent = false) = 0;
		virtual ER_RHI_GPUTexture* AddOrGetGPUTextureFromCache(const std::wstring& aFullPath, bool* didExist = nullptr, bool is3D = false, bool skipFallback = false, bool* statusFlag = nullptr, bool isSilent = false) = 0;
		virtual void AddGPUTextureToCache(const std::wstring& aFullPath, ER_RHI_GPUTexture* aTexture) = 0;
		virtual bool RemoveGPUTextureFromCache(const std::wstring& aFullPath, bool removeKey = false) = 0;
		virtual void ReplaceGPUTextureFromCache(const std::wstring& aFullPath, ER_RHI_GPUTexture* aTex) = 0; // WARNING: dangerous!
		virtual bool IsGPUTextureInCache(const std::wstring& aFullPath) = 0;

		const ER_CoreServicesContainer& GetServices() const { return mCoreServices; }
		ER_Sandbox* GetLevel() { return mCurrentSandbox; }
		ER_CPUProfiler* CPUProfiler() const { return mCPUProfiler; }
		ER_RHI* GetRHI() { return mRHI; }

		UINT GetFrameIndex() { return mFrameIndex; }
		const ER_CoreTime& GetCoreTime() { return mCoreTime; }
		double GetCoreTotalTime() { return mCoreTime.TotalCoreTime(); }
	protected:
		virtual void InitializeWindow();
		virtual void Shutdown();

		ER_CoreClock mCoreClock;
		ER_CoreTime mCoreTime;
		ER_CPUProfiler* mCPUProfiler = nullptr;
		ER_Sandbox* mCurrentSandbox = nullptr;
		ER_RHI* mRHI = nullptr;

		// Some core engine systems can be stored in both containers below:
		// Note: This is old legacy code and perhaps I will change it one day (I would prefer to keep RTTI though...).
		ER_CoreServicesContainer mCoreServices; // can be used everywhere in the engine via RTTI
		std::vector<ER_CoreComponent*> mCoreComponents; // can be used only within ER_Core for easy iterative operations: update/init of the systems

		HINSTANCE mInstance;
		std::wstring mWindowClass;
		std::wstring mWindowTitle;
		int mShowCommand;

		HWND mWindowHandle;
		WNDCLASSEX mWindow;

		UINT mScreenWidth;
		UINT mScreenHeight;

		ER_RHI_Viewport mMainViewport;
		ER_RHI_Rect mMainRect;

		bool mIsFullscreen;
		UINT mFrameIndex = 0;
	private:
		ER_Core(const ER_Core& rhs);
		ER_Core& operator=(const ER_Core& rhs);

		POINT CenterWindow(int windowWidth, int windowHeight);
		static LRESULT WINAPI WndProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);
	};
}