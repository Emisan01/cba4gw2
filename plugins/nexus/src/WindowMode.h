#pragma once
#include <windows.h>
#include <dxgi.h>

namespace cba
{
	enum class WindowMode
	{
		Composited,           // windowed or borderless — DWM is active, filter works
		ExclusiveFullscreen,  // DXGI exclusive fullscreen — DWM bypassed, filter does NOT work
		Unknown               // couldn't determine yet (e.g. before first frame)
	};

	// Reads the ground truth directly from the swapchain Nexus already hands
	// us in AddonAPI::SwapChain — IDXGISwapChain::GetFullscreenState() is
	// exactly the flag DWM itself uses to decide whether it composites this
	// window or gets bypassed. No window-style heuristics, no hooking.
	WindowMode DetectWindowMode(IDXGISwapChain* aSwapChain);

	const char* ToDisplayString(WindowMode aMode);
}
