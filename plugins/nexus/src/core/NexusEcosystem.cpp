#include "NexusEcosystem.h"

namespace cba
{
	NexusEcosystem& NexusEcosystem::Get()
	{
		static NexusEcosystem instance;
		return instance;
	}

	void NexusEcosystem::Initialize()
	{
		Update();
	}

	void NexusEcosystem::Update()
	{
		auto now = std::chrono::steady_clock::now();
		// Throttle checks to once every 2 seconds
		if (std::chrono::duration_cast<std::chrono::seconds>(now - mLastCheckTime).count() < 2)
			return;

		mLastCheckTime = now;

		mIsArcDPSLoaded.store(GetModuleHandleW(L"d3d11.dll") != nullptr || GetModuleHandleW(L"ArcDPS.dll") != nullptr);
		mIsFastLoadLoaded.store(GetModuleHandleW(L"Fast_Load.dll") != nullptr);
	}

	void NexusEcosystem::Shutdown()
	{
		// Nothing to tear down - removed 2026-09-15, along with the
		// ArcDPS ini/theme sync attempt this used to join a worker thread
		// for. That sync was already disabled and untested (see git
		// history if it's ever revisited); the thread it left running did
		// nothing but poll a shutdown flag. Kept as a no-op call, not
		// deleted outright, so ModuleMain.cpp's AddonUnload lifecycle stays
		// symmetric with Initialize/Update above.
	}
}
