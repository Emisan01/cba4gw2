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
		float r, g, b;            // Search color (0-1)
		uint8_t repR, repG, repB; // Replacement color
	};

	class HybridScanner
	{
	public:
		HybridScanner();
		~HybridScanner();

		void Initialize();
		void Shutdown();

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
		std::vector<uint8_t> mPreviousFrameRgba;
		float mMotionFader = 1.0f;
		
		bool mOverlayReady = false;
		
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
