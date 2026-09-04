#include "WindowMode.h"

namespace cba
{
	WindowMode DetectWindowMode(IDXGISwapChain* aSwapChain)
	{
		if (!aSwapChain)
			return WindowMode::Unknown;

		BOOL isFullscreen = FALSE;
		HRESULT hr = aSwapChain->GetFullscreenState(&isFullscreen, nullptr);

		if (FAILED(hr))
			return WindowMode::Unknown;

		return isFullscreen ? WindowMode::ExclusiveFullscreen : WindowMode::Composited;
	}

	const char* ToDisplayString(WindowMode aMode)
	{
		switch (aMode)
		{
			case WindowMode::Composited:          return "Windowed / Borderless - filter active";
			case WindowMode::ExclusiveFullscreen: return "Exclusive Fullscreen - filter INACTIVE, switch display mode in GW2 options";
			default:                              return "Unknown";
		}
	}
}
