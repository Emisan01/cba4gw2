#include "HybridScanner.h"
#include <cmath>
#include <algorithm>

namespace cba
{
	HybridScanner& GetHybridScanner()
	{
		static HybridScanner instance;
		return instance;
	}

	HybridScanner::HybridScanner()
	{
	}

	HybridScanner::~HybridScanner()
	{
		Shutdown();
	}

	void HybridScanner::SetHighlighterParams(bool aEnable, const std::vector<TargetColor>& aTargets, float aTol)
	{
		std::lock_guard<std::mutex> lock(mProblemsMutex);
		mHighlighterEnabled = aEnable;
		mHighlighterTargets = aTargets;
		mHighlighterTolerance = aTol;
	}

	void HybridScanner::SetScanRegion(float u1, float v1, float u2, float v2)
	{
		std::lock_guard<std::mutex> lock(mProblemsMutex);
		// Robustness Check: Ensure bounds are valid and clamped
		mScanU1 = (std::max)(0.0f, (std::min)(u1, 1.0f));
		mScanV1 = (std::max)(0.0f, (std::min)(v1, 1.0f));
		mScanU2 = (std::max)(0.0f, (std::min)(u2, 1.0f));
		mScanV2 = (std::max)(0.0f, (std::min)(v2, 1.0f));
		
		if (mScanU1 > mScanU2) std::swap(mScanU1, mScanU2);
		if (mScanV1 > mScanV2) std::swap(mScanV1, mScanV2);
	}


	void HybridScanner::SetNexusLinkData(bool aIsCameraMoving, bool aIsGameplay, bool aIsInCombat)
	{
		std::lock_guard<std::mutex> lock(mProblemsMutex);
		mNexusIsCameraMoving = aIsCameraMoving;
		mNexusIsGameplay = aIsGameplay;
		mMumbleIsInCombat = aIsInCombat;
	}

	void HybridScanner::UpdateHardwareLoad(float aFps)
	{
		// Aggressive throttling for Commander Tags (don't need 60 FPS)
		// Performance boost: reduce CPU load significantly
		if (aFps > 45.0f) {
			mDynamicScanIntervalMs = 100; // ~10 FPS scan (down from 33ms)
		} else if (aFps > 20.0f) {
			mDynamicScanIntervalMs = 150; // ~6.6 FPS scan (down from 66ms)
		} else {
			mDynamicScanIntervalMs = 200; // ~5 FPS scan (down from 150ms)
		}
	}

	void HybridScanner::Initialize()
	{
		// Check-and-set must be one atomic step (compare_exchange), not two
		// separate statements - Initialize() can now be called concurrently
		// from two different threads (the main/render thread via Reset
		// Filter, and the Watchdog thread's self-heal, both added 2026-09-09).
		// The old "if (mRunning) return; mRunning = true;" let both callers
		// pass the check before either flipped the flag, so both would then
		// assign to mThread - and std::thread's move-assignment calls
		// std::terminate() (hard process crash) if the target already holds
		// a joinable thread. compare_exchange_strong makes only one caller
		// win the race.
		bool expected = false;
		if (!mRunning.compare_exchange_strong(expected, true)) return;

		// Also required even without any race: if the previous worker thread
		// exited on its own (the per-iteration catch still couldn't save it -
		// see WorkerThread's comment), mThread is still "joinable" even
		// though the OS thread already finished. Reassigning mThread without
		// reaping it first hits the exact same std::terminate() crash.
		// join() on an already-finished thread returns immediately.
		std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
		if (mThread.joinable()) mThread.join();

		mThread = std::thread(&HybridScanner::WorkerThread, this);
	}

	void HybridScanner::Shutdown()
	{
		mRunning = false;
		mDataCond.notify_all();

		{
			// Same mLifecycleMutex Initialize() takes - without this, a
			// concurrent Initialize() call (e.g. the Watchdog's self-heal)
			// could be join()ing or reassigning mThread on another thread at
			// the same moment (found in the 2026-09-09 codebase review).
			std::lock_guard<std::mutex> lifecycleLock(mLifecycleMutex);
			if (mThread.joinable()) {
				mThread.join();
			}
		}

		if (mStagingTexture) {
			mStagingTexture->Release();
			mStagingTexture = nullptr;
		}
		if (mOverlayTexture) {
			mOverlayTexture->Release();
			mOverlayTexture = nullptr;
		}
		if (mOverlaySRV) {
			mOverlaySRV->Release();
			mOverlaySRV = nullptr;
		}
	}

	void HybridScanner::SetEnabled(bool aEnabled)
	{
		mEnabled = aEnabled;
		if (!aEnabled) {
			std::lock_guard<std::mutex> lock(mProblemsMutex);
			mProblems.clear();
		}
	}

	ID3D11ShaderResourceView* HybridScanner::GetOverlaySRV(int& outWidth, int& outHeight)
	{
		std::lock_guard<std::mutex> lock(mProblemsMutex);
		outWidth = mTexWidth;
		outHeight = mTexHeight; // Fullscreen draw size
		return mOverlaySRV;
	}

	double HybridScanner::RelativeLuminance(uint8_t r, uint8_t g, uint8_t b)
	{
		auto f = [](uint8_t c) {
			double v = c / 255.0;
			return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
		};
		return 0.2126 * f(r) + 0.7152 * f(g) + 0.0722 * f(b);
	}

	double HybridScanner::WcagContrast(double lum1, double lum2)
	{
		double l1 = std::max(lum1, lum2);
		double l2 = std::min(lum1, lum2);
		return (l1 + 0.05) / (l2 + 0.05);
	}

	void HybridScanner::AnalyzeBuffer(const std::vector<uint8_t>& aRgba, int aWidth, int aHeight, DXGI_FORMAT aFormat)
	{
		int outWidth = aWidth / 4;
		int outHeight = aHeight / 4;
		int requiredSize = outWidth * outHeight * 4;
		
		if (mThreadTempBuffer.size() != requiredSize) {
			mThreadTempBuffer.resize(requiredSize, 0);
		}
		if (mThreadBlurBuffer.size() != requiredSize) {
			mThreadBlurBuffer.resize(requiredSize, 0);
		}
		
		// Fast zero-memory
		std::memset(mThreadTempBuffer.data(), 0, requiredSize);
		std::memset(mThreadBlurBuffer.data(), 0, requiredSize);
		
		auto& tempBuffer = mThreadTempBuffer;
		auto& blurBuffer = mThreadBlurBuffer;

		// Local copies for thread safety
		bool highlighterEnabled = false;
		std::vector<TargetColor> targets;
		float tolerance = 0;
		float su1 = 0, sv1 = 0, su2 = 1, sv2 = 1;
		{
			std::lock_guard<std::mutex> lock(mProblemsMutex);
			highlighterEnabled = mHighlighterEnabled;
			targets = mHighlighterTargets;
			tolerance = mHighlighterTolerance;
			su1 = mScanU1; sv1 = mScanV1; su2 = mScanU2; sv2 = mScanV2;
		}

		bool isBgra = (aFormat == DXGI_FORMAT_B8G8R8A8_UNORM || aFormat == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);

		int startX = std::max(0, static_cast<int>(su1 * outWidth));
		int startY = std::max(0, static_cast<int>(sv1 * outHeight));
		int endX = std::min(outWidth, static_cast<int>(su2 * outWidth));
		int endY = std::min(outHeight, static_cast<int>(sv2 * outHeight));

		bool isCameraMoving = false;
		bool isGameplay = true;
		{
			std::lock_guard<std::mutex> lock(mProblemsMutex);
			isCameraMoving = mNexusIsCameraMoving;
			isGameplay = mNexusIsGameplay;
		}

		if (!isGameplay) {
			// Save CPU by not scanning in loading screens / main menu
			std::lock_guard<std::mutex> lock(mProblemsMutex);
			mFinalOverlayBuffer.clear(); // Clear overlay
			mOverlayReady = true;
			return;
		}

		// --- Motion Detection (Nexus API Camera Sensor) ---
		if (isCameraMoving) {
			mMotionFader -= 0.2f; // Fade out fast
		} else {
			mMotionFader += 0.05f; // Fade in slow
		}
		if (mMotionFader < 0.0f) mMotionFader = 0.0f;
		if (mMotionFader > 1.0f) mMotionFader = 1.0f;
		
		// A full-frame copy into mPreviousFrameRgba used to happen right
		// here, every scan tick (every ~100-200ms) - a multi-megabyte
		// allocation+copy for a field nothing ever read back. The old SAD
		// (Sum of Absolute Differences) motion-detection algorithm that
		// used it was already replaced by the Nexus isCameraMoving flag
		// above; the buffer itself was never removed (found in the
		// 2026-09-09 codebase review). Removed along with the field.

		struct MatchResult {
			int x, y;
			uint8_t repR, repG, repB;
		};
		std::vector<MatchResult> matches;
		matches.reserve(1000); // Pre-allocate to avoid allocations in loop

		for (int y = startY; y < endY; ++y) {
			for (int x = startX; x < endX; ++x) {
				int srcY = y * 4;
				int srcX = x * 4;
				int idx1 = (srcY * aWidth + srcX) * 4;
				
				uint8_t r = isBgra ? aRgba[idx1+2] : aRgba[idx1];
				uint8_t g = aRgba[idx1+1];
				uint8_t b = isBgra ? aRgba[idx1] : aRgba[idx1+2];
				
				bool highlight = false;
				uint8_t repR = 255, repG = 255, repB = 255;

				if (highlighterEnabled && !targets.empty()) {
					// Check all target colors, keep the CLOSEST match rather than
					// the first one in list order. Previously this broke out of
					// the loop on the first target whose tolerance the pixel fell
					// within - meaning a pixel sitting between two overlapping
					// targets (e.g. a Filter Lab target and an auto-derived
					// Commander Tag target with similar hues) got assigned
					// arbitrarily by list order, not by which target it's
					// actually closer to. Still picks exactly one target's fixed
					// rep color (not a blend) - the cluster-grouping logic below
					// groups matches by exact repR/repG/repB equality, so a true
					// multi-target blend would fragment every cluster into
					// single-pixel noise and break Commander Tag detection
					// entirely. A real weighted-composite (Roman-instrument-
					// style, each target contributing proportionally) would need
					// that clustering approach reworked too - out of scope for
					// this pass, noted in CLAUDE.md as a follow-up.
					float bestDist = -1.0f;
					for (const auto& target : targets) {
						float dr = static_cast<float>(r) - (target.r * 255.0f);
						float dg = static_cast<float>(g) - (target.g * 255.0f);
						float db = static_cast<float>(b) - (target.b * 255.0f);
						float dist = std::sqrt(dr*dr + dg*dg + db*db);

						float effTol = (target.tolerance > 0.001f) ? target.tolerance : tolerance;
						float coreDist = 255.0f * effTol;
						float maxDist = coreDist * (1.0f + std::max(0.0f, target.diffusion));

						if (dist < maxDist && (bestDist < 0.0f || dist < bestDist)) {
							bestDist = dist;
							highlight = true;
							float alpha = 1.0f;
							if (dist > coreDist && target.diffusion > 0.001f) {
								float t = (dist - coreDist) / (maxDist - coreDist);
								alpha = 0.5f * (1.0f + std::cos(t * 3.1415926535f));
							}
							repR = static_cast<uint8_t>(std::clamp(target.repR * alpha, 0.0f, 255.0f));
							repG = static_cast<uint8_t>(std::clamp(target.repG * alpha, 0.0f, 255.0f));
							repB = static_cast<uint8_t>(std::clamp(target.repB * alpha, 0.0f, 255.0f));
						}
					}
				} else if (!highlighterEnabled) {
					// Default WCAG Contrast Search
					if (srcX < aWidth - 4) {
						int idx2 = (srcY * aWidth + (srcX + 4)) * 4; 
						uint8_t r2 = isBgra ? aRgba[idx2+2] : aRgba[idx2];
						uint8_t g2 = aRgba[idx2+1];
						uint8_t b2 = isBgra ? aRgba[idx2] : aRgba[idx2+2];

						double l1 = RelativeLuminance(r, g, b);
						double l2 = RelativeLuminance(r2, g2, b2);

						if (std::abs(l1 - l2) > 0.05) {
							double ratio = WcagContrast(l1, l2);
							if (ratio < 2.5) {
								highlight = true;
								repR = 50; repG = 200; repB = 255;
							}
						}
					}
				}

				if (highlight) {
					if (highlighterEnabled && !targets.empty()) {
						matches.push_back({x, y, repR, repG, repB});
					} else {
						int outIdx = (y * outWidth + x) * 4;
						float timeSeconds = GetTickCount64() / 1000.0f;
						float pulse = 0.8f + 0.2f * std::sin(timeSeconds * 5.0f);

						tempBuffer[outIdx]   = static_cast<uint8_t>(repR * pulse);
						tempBuffer[outIdx+1] = static_cast<uint8_t>(repG * pulse);
						tempBuffer[outIdx+2] = static_cast<uint8_t>(repB * pulse);
						tempBuffer[outIdx+3] = static_cast<uint8_t>(255 * mMotionFader);
					}
				}
			}
		}

		// Phase 2: Cluster logic for Commander Tags
		if (highlighterEnabled && !targets.empty() && !matches.empty()) {
			struct MatchCluster {
				int minX, maxX;
				int minY, maxY;
				int count;
				uint8_t repR, repG, repB;
				std::vector<const MatchResult*> pixels;
			};
			
			std::vector<MatchCluster> clusters;
			clusters.reserve(50);
			
			for (const auto& m : matches) {
				bool added = false;
				for (auto& c : clusters) {
					// Check if same color
					if (c.repR == m.repR && c.repG == m.repG && c.repB == m.repB) {
						// Check distance (max 15 pixels gap for downscaled buffer)
						int dx = std::max(0, std::max(c.minX - m.x, m.x - c.maxX));
						int dy = std::max(0, std::max(c.minY - m.y, m.y - c.maxY));
						if (dx <= 15 && dy <= 15) {
							c.minX = std::min(c.minX, m.x);
							c.maxX = std::max(c.maxX, m.x);
							c.minY = std::min(c.minY, m.y);
							c.maxY = std::max(c.maxY, m.y);
							c.count++;
							c.pixels.push_back(&m);
							added = true;
							break;
						}
					}
				}
				
				if (!added) {
					if (clusters.size() < 100) { // Limit max clusters to prevent CPU spikes
						MatchCluster c;
						c.minX = c.maxX = m.x;
						c.minY = c.maxY = m.y;
						c.count = 1;
						c.repR = m.repR;
						c.repG = m.repG;
						c.repB = m.repB;
						c.pixels.push_back(&m);
						clusters.push_back(c);
					}
				}
			}
			
			// Phase 3: Filter and Draw Valid Clusters
			float timeSeconds = GetTickCount64() / 1000.0f;
			float pulse = 0.8f + 0.2f * std::sin(timeSeconds * 5.0f);
			
			for (const auto& c : clusters) {
				int width = c.maxX - c.minX + 1;
				int height = c.maxY - c.minY + 1;
				
				// A valid commander tag / marker in downscaled buffer:
				// Must not be too huge (e.g. fire/ground effect)
				// Must not be a single pixel (e.g. random artifact)
				if (width >= 2 && height >= 2 && width <= 60 && height <= 60 && c.count >= 4) {
					// Draw pixels for this cluster
					for (const auto* p : c.pixels) {
						int outIdx = (p->y * outWidth + p->x) * 4;
						tempBuffer[outIdx]   = static_cast<uint8_t>(c.repR * pulse);
						tempBuffer[outIdx+1] = static_cast<uint8_t>(c.repG * pulse);
						tempBuffer[outIdx+2] = static_cast<uint8_t>(c.repB * pulse);
						tempBuffer[outIdx+3] = static_cast<uint8_t>(255 * mMotionFader); // Fade out during fast motion
					}
				}
			}
		}

		// Apply blur in 2 passes (Horizontal then Vertical) on downscaled buffer
		int radius = 4;

		// Horizontal Pass
		for (int y = 0; y < outHeight; ++y) {
			for (int x = 0; x < outWidth; ++x) {
				int sumR = 0, sumG = 0, sumB = 0, sumA = 0;
				int count = 0;
				for (int k = -radius; k <= radius; ++k) {
					int nx = x + k;
					if (nx >= 0 && nx < outWidth) {
						int nIdx = (y * outWidth + nx) * 4;
						sumR += tempBuffer[nIdx];
						sumG += tempBuffer[nIdx+1];
						sumB += tempBuffer[nIdx+2];
						sumA += tempBuffer[nIdx+3];
						count++;
					}
				}
				int outIdx = (y * outWidth + x) * 4;
				blurBuffer[outIdx] = sumR / count;
				blurBuffer[outIdx+1] = sumG / count;
				blurBuffer[outIdx+2] = sumB / count;
				blurBuffer[outIdx+3] = sumA / count; 
			}
		}

		// Vertical Pass
		for (int y = 0; y < outHeight; ++y) {
			for (int x = 0; x < outWidth; ++x) {
				int sumR = 0, sumG = 0, sumB = 0, sumA = 0;
				int count = 0;
				for (int k = -radius; k <= radius; ++k) {
					int ny = y + k;
					if (ny >= 0 && ny < outHeight) {
						int nIdx = (ny * outWidth + x) * 4;
						sumR += blurBuffer[nIdx];
						sumG += blurBuffer[nIdx+1];
						sumB += blurBuffer[nIdx+2];
						sumA += blurBuffer[nIdx+3];
						count++;
					}
				}
				int outIdx = (y * outWidth + x) * 4;
				tempBuffer[outIdx] = sumR / count;
				tempBuffer[outIdx+1] = sumG / count;
				tempBuffer[outIdx+2] = sumB / count;
				// Soften alpha to 80% so game underneath is visible
				tempBuffer[outIdx+3] = (uint8_t)((sumA / count) * 0.8);
			}
		}

		std::lock_guard<std::mutex> lock(mProblemsMutex);
		mFinalOverlayBuffer = std::move(tempBuffer);
		mOverlayReady = true;
	}

	void HybridScanner::ScanFrame(IDXGISwapChain* aSwapChain)
	{
		if (!mEnabled || !aSwapChain) return;

		ID3D11Device* device = nullptr;
		if (FAILED(aSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&device))) return;

		ID3D11DeviceContext* context = nullptr;
		device->GetImmediateContext(&context);

		// 1. Check if background thread has a new overlay ready to upload
		{
			std::lock_guard<std::mutex> lock(mProblemsMutex);
			if (mOverlayReady) {


				if (mOverlayTexture && mTexWidth > 0 && mTexHeight > 0) {
					// We must upload row by row if RowPitch doesn't exactly match width*4
					D3D11_MAPPED_SUBRESOURCE mapped;
					if (SUCCEEDED(context->Map(mOverlayTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
						uint8_t* dest = static_cast<uint8_t*>(mapped.pData);
						const uint8_t* src = mFinalOverlayBuffer.data();
						
						int outW = mTexWidth / 4;
						int outH = mTexHeight / 4;

						if (src && dest && mFinalOverlayBuffer.size() >= outW * outH * 4) {
							for (int y = 0; y < outH; ++y) {
								memcpy(dest + y * mapped.RowPitch, src + y * outW * 4, outW * 4);
							}
						}
						context->Unmap(mOverlayTexture, 0);
					}
				}
				mOverlayReady = false;
			}
		}

		DWORD now = GetTickCount();

		// 2. Capture a new frame if we are idle and it's time
		if (mScanState == ScanState::Idle && (now - mLastScanTime >= mDynamicScanIntervalMs)) {
			ID3D11Texture2D* backBuffer = nullptr;
			if (FAILED(aSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
				context->Release();
				device->Release();
				return;
			}

			D3D11_TEXTURE2D_DESC desc;
			backBuffer->GetDesc(&desc);

			if (!mStagingTexture || mTexWidth != desc.Width || mTexHeight != desc.Height) {
				if (mStagingTexture) {
					mStagingTexture->Release();
					mStagingTexture = nullptr;
				}
				if (mOverlayTexture) {
					mOverlayTexture->Release();
					mOverlayTexture = nullptr;
				}
				if (mOverlaySRV) {
					mOverlaySRV->Release();
					mOverlaySRV = nullptr;
				}
				
				desc.BindFlags = 0;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				desc.Usage = D3D11_USAGE_STAGING;
				desc.MiscFlags = 0;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				
				if (FAILED(device->CreateTexture2D(&desc, nullptr, &mStagingTexture))) {
					backBuffer->Release();
					context->Release();
					device->Release();
					return;
				}
				mTexWidth = desc.Width;
				mTexHeight = desc.Height;
				
				// Create Dynamic Texture for Overlay (Downscaled 1/4)
				D3D11_TEXTURE2D_DESC oDesc;
				oDesc.Width = mTexWidth / 4;
				oDesc.Height = mTexHeight / 4;
				oDesc.MipLevels = 1;
				oDesc.ArraySize = 1;
				oDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				oDesc.SampleDesc.Count = 1;
				oDesc.SampleDesc.Quality = 0;
				oDesc.Usage = D3D11_USAGE_DYNAMIC;
				oDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				oDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				oDesc.MiscFlags = 0;
				
				if (SUCCEEDED(device->CreateTexture2D(&oDesc, nullptr, &mOverlayTexture))) {
					device->CreateShaderResourceView(mOverlayTexture, nullptr, &mOverlaySRV);
				}
			}

			D3D11_TEXTURE2D_DESC bbDesc;
			backBuffer->GetDesc(&bbDesc);
			if (bbDesc.SampleDesc.Count > 1) {
				context->ResolveSubresource(mStagingTexture, 0, backBuffer, 0, bbDesc.Format);
			} else {
				context->CopyResource(mStagingTexture, backBuffer);
			}
			mSwapChainFormat = bbDesc.Format;
			
			backBuffer->Release();
			
			mScanState = ScanState::WaitingForGPU;
			mScanRequestedTime = now;
		}
		else if (mScanState == ScanState::WaitingForGPU) {
			// Wait 16ms (1 frame @ 60Hz) before mapping to ensure GPU is done and eliminate latency
			if (now - mScanRequestedTime >= 16) {
				LARGE_INTEGER start, end, freq;
				QueryPerformanceFrequency(&freq);
				QueryPerformanceCounter(&start);

				D3D11_MAPPED_SUBRESOURCE mapped;
				if (SUCCEEDED(context->Map(mStagingTexture, 0, D3D11_MAP_READ, 0, &mapped))) {
					if (mapped.pData && mapped.RowPitch >= mTexWidth * 4) {
						std::unique_lock<std::mutex> lock(mDataMutex);
						int numPixels = mTexWidth * mTexHeight;
						if (mPendingBuffer.size() != numPixels * 4) {
							mPendingBuffer.resize(numPixels * 4);
						}
						
						const uint8_t* src = static_cast<const uint8_t*>(mapped.pData);
						for (int y = 0; y < mTexHeight; ++y) {
							memcpy(&mPendingBuffer[y * mTexWidth * 4], src + y * mapped.RowPitch, mTexWidth * 4);
						}
						mPendingWidth = mTexWidth;
						mPendingHeight = mTexHeight;
						mHasNewData = true;
						
						lock.unlock();
						mDataCond.notify_one();
					}
					context->Unmap(mStagingTexture, 0);
				}

				QueryPerformanceCounter(&end);
				mMainThreadImpactMs = (float)((end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart);
				
				mScanState = ScanState::Idle;
				mLastScanTime = now;
			}
		}

		context->Release();
		device->Release();
	}

	// Standalone wrapper to avoid C2712 (object unwinding conflict with __try)
	static void AnalyzeBufferSEH(HybridScanner* scanner, const std::vector<uint8_t>* buffer, int w, int h, DXGI_FORMAT fmt, int* outErrorCount)
	{
		__try {
			// This call is wrapped inside SEH, so Access Violations inside are caught
			scanner->AnalyzeBuffer(*buffer, w, h, fmt);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			(*outErrorCount)++;
		}
	}

	void HybridScanner::WorkerThread()
	{
		// The try/catch used to wrap this entire while loop, once, on the
		// outside. That meant any single C++ exception thrown anywhere in one
		// iteration - e.g. from activating a Filter Lab instance mid-session -
		// unwound past the loop entirely and ended the thread for the rest of
		// the process: mRunning was never reset to false, so even the
		// Initialize() guard ("if (mRunning) return;") believed the scanner
		// was still alive and refused to restart it. No amount of toggling
		// Enabled or clicking Reset Filter could bring it back (2026-09-09,
		// Emi's fullscreen test session - "schwupps ging nichts mehr").
		//
		// Fix: catch per-iteration so a single bad frame can't kill the
		// thread, and always leave mRunning accurate on the way out so
		// IsRunning() tells the truth and a caller can Initialize() again.
		std::vector<uint8_t> localBuffer;
		int localWidth = 0;
		int localHeight = 0;

		while (mRunning) {
			try
			{
				std::unique_lock<std::mutex> lock(mDataMutex);
				mDataCond.wait(lock, [this] { return mHasNewData || !mRunning; });

				if (!mRunning) break;

				// Swap buffers to minimize lock time
				localBuffer.swap(mPendingBuffer);
				localWidth = mPendingWidth;
				localHeight = mPendingHeight;
				mHasNewData = false;

				DXGI_FORMAT currentFormat = DXGI_FORMAT_UNKNOWN;
				{
					std::lock_guard<std::mutex> fLock(mProblemsMutex);
					currentFormat = mSwapChainFormat;
				}

				lock.unlock();

				LARGE_INTEGER start, end, freq;
				QueryPerformanceFrequency(&freq);
				QueryPerformanceCounter(&start);

				int errorCount = 0;
				AnalyzeBufferSEH(this, &localBuffer, localWidth, localHeight, currentFormat, &errorCount);
				if (errorCount > 0) mErrorCount += errorCount;

				QueryPerformanceCounter(&end);
				mLastScanTimeMs = (float)((end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart);
			}
			catch (const std::exception&)
			{
				++mErrorCount;
			}
			catch (...)
			{
				++mErrorCount;
			}
		}
		mRunning = false;
	}
}
