#pragma once
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace cba
{
	struct WcagProblem
	{
		int x, y;
	};

	struct TargetColor 
	{
		float r = 0.0f, g = 0.0f, b = 0.0f; // Search color (0-1)
		uint8_t repR = 255, repG = 255, repB = 255; // Replacement color
		float tolerance = 0.0f;             // Individual color tolerance (0 = use global, >0 = individual)
		float diffusion = 0.0f;             // Soft diffusion falloff (0.0 = sharp cut, 1.0 = smooth fade)
		int actionType = 0;                 // 0 = replace, 1 = complement, 2 = invert, 3 = luminance boost
		// Filter Layer Matrix rank (2026-09-11, core/FilterLayers.h) - lower
		// = higher priority. AnalyzeBuffer's matching now picks the closest
		// match within the BEST-priority layer that has any match for a
		// given pixel, instead of the single global closest match across
		// every target regardless of source - see AnalyzeBuffer's comment.
		int layerPriority = 0;
	};

	class HybridScanner
	{
	public:
		HybridScanner();
		~HybridScanner();

		void Initialize();
		void Shutdown();

		// True while the background worker thread is actually alive. Can go
		// false on its own even without Shutdown() being called, if the
		// thread's per-iteration catch still couldn't save it from something
		// fatal (see WorkerThread's comment) - the Watchdog and Reset Filter
		// both check this to auto-restart the scanner rather than leaving it
		// permanently dead for the rest of the process.
		bool IsRunning() const { return mRunning; }

		// Enables or disables the hybrid scanner
		void SetEnabled(bool aEnabled);

		// Called from the Main Thread (AddonRender)
		// Safely captures the frame occasionally and dispatches it to the background thread.
		void ScanFrame(IDXGISwapChain* aSwapChain);

		// Returns the overlay texture to be drawn by ImGui. Thread-safe.
		ID3D11ShaderResourceView* GetOverlaySRV(int& outWidth, int& outHeight);

		void SetHighlighterParams(bool aEnable, const std::vector<TargetColor>& aTargets, float aTol);
		void SetScanRegion(float u1, float v1, float u2, float v2);
		
		void SetNexusLinkData(bool aIsCameraMoving, bool aIsGameplay, bool aIsInCombat = false);
		
		// Adjusts internal scan interval based on current game performance
		void UpdateHardwareLoad(float aFps);

		// Debug Metrics
		float GetLastScanTimeMs() const { return mLastScanTimeMs; }
		float GetMainThreadImpactMs() const { return mMainThreadImpactMs; }
		int GetErrorCount() const { return mErrorCount; }
		bool GetMumbleIsInCombat() const { return mMumbleIsInCombat; }

		void AnalyzeBuffer(const std::vector<uint8_t>& aRgba, int aWidth, int aHeight, DXGI_FORMAT aFormat);

	private:
		void WorkerThread();
		double RelativeLuminance(uint8_t r, uint8_t g, uint8_t b);
		double WcagContrast(double lum1, double lum2);

		ID3D11Texture2D* mStagingTexture = nullptr;
		
		ID3D11Texture2D* mOverlayTexture = nullptr;
		ID3D11ShaderResourceView* mOverlaySRV = nullptr;

		std::atomic<bool> mRunning = false;
		std::atomic<bool> mEnabled = false;
		std::thread mThread;
		// Guards every mThread.join()/reassignment in Initialize() and
		// Shutdown() against each other (found in the 2026-09-09 codebase
		// review): Initialize() was hardened with compare_exchange_strong on
		// mRunning so two Initialize() callers can't race, but Shutdown()
		// still did a plain join() with no synchronization against a
		// concurrent Initialize() (main thread's Reset Filter self-heal vs.
		// AddonUnload/DLL_PROCESS_DETACH's Shutdown) - two threads touching
		// the same std::thread object with no lock is undefined behavior.
		std::mutex mLifecycleMutex;

		// Data passing to the worker thread
		std::mutex mDataMutex;
		std::condition_variable mDataCond;
		std::vector<uint8_t> mPendingBuffer;
		int mPendingWidth = 0;
		int mPendingHeight = 0;
		bool mHasNewData = false;

		std::mutex mProblemsMutex;

		// Highlighter Params
		bool mHighlighterEnabled = false;
		std::vector<TargetColor> mHighlighterTargets;
		float mHighlighterTolerance = 0.15f;
		
		
		// Region Scanner
		float mScanU1 = 0.0f, mScanV1 = 0.0f, mScanU2 = 1.0f, mScanV2 = 1.0f;
		
		// Nexus Link Data
		bool mNexusIsCameraMoving = false;
		bool mNexusIsGameplay = true;
		bool mMumbleIsInCombat = false;
		
		// Metrics
		float mLastScanTimeMs = 0.0f;
		float mMainThreadImpactMs = 0.0f;
		int mErrorCount = 0;
		
		DXGI_FORMAT mSwapChainFormat = DXGI_FORMAT_UNKNOWN;
		std::vector<WcagProblem> mProblems; // Keeping for reference if needed
		std::vector<uint8_t> mFinalOverlayBuffer;
		
		// Memory pools to avoid allocations in worker thread
		std::vector<uint8_t> mThreadTempBuffer;
		std::vector<uint8_t> mThreadBlurBuffer;
		
		// Motion Detection
		float mMotionFader = 1.0f;
		bool mOverlayReady = false;
		int mOverlayReadyW = 0;
		int mOverlayReadyH = 0;
		
		int mTexWidth = 0;
		int mTexHeight = 0;
		
		enum class ScanState {
			Idle,
			WaitingForGPU
		};
		ScanState mScanState = ScanState::Idle;
		ULONGLONG mLastScanTime = 0;
		ULONGLONG mScanRequestedTime = 0;
		
		// Dynamic Throttle
		ULONGLONG mDynamicScanIntervalMs = 100;
	};

	// Global singleton access
	HybridScanner& GetHybridScanner();
}
