#pragma once
#include <windows.h>
#include <atomic>
#include <chrono>
#include <thread>

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
		NexusEcosystem();
		~NexusEcosystem();

		void IniSyncWorker();
		void InjectArcDPSIni();
		void SyncArcDpsColors();

		std::atomic<bool> mIsArcDPSLoaded{ false };
		std::atomic<bool> mIsFastLoadLoaded{ false };

		std::atomic<bool> mWorkerRunning{ false };
		std::atomic<bool> mShuttingDown{ false };
		std::thread mIniSyncThread;

		std::chrono::steady_clock::time_point mLastCheckTime{};
	};
}
