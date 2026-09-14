#pragma once
#include <windows.h>
#include <atomic>
#include <chrono>

namespace cba
{
	class NexusEcosystem
	{
	public:
		static NexusEcosystem& Get();

		void Initialize();
		void Update(); // Call periodically or on frame
		void Shutdown(); // Triggered during AddonUnload

		bool IsArcDPSLoaded() const { return mIsArcDPSLoaded.load(); }
		bool IsFastLoadLoaded() const { return mIsFastLoadLoaded.load(); }

	private:
		NexusEcosystem() = default;

		std::atomic<bool> mIsArcDPSLoaded{ false };
		std::atomic<bool> mIsFastLoadLoaded{ false };

		std::chrono::steady_clock::time_point mLastCheckTime{};
	};
}
