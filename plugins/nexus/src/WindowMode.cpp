#include "WindowMode.h"

namespace cba
{
	WindowMode DetectWindowMode(IDXGISwapChain* aSwapChain)
	{
		if (!aSwapChain)
			return WindowMode::Unknown;

		__try
		{
			BOOL isFullscreen = FALSE;
			HRESULT hr = aSwapChain->GetFullscreenState(&isFullscreen, nullptr);

			if (FAILED(hr))
				return WindowMode::Unknown;

			return isFullscreen ? WindowMode::ExclusiveFullscreen : WindowMode::Composited;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return WindowMode::Unknown;
		}
	}

	bool DetectHdrColorSpace(IDXGISwapChain* aSwapChain)
	{
		if (!aSwapChain)
			return false;

		__try
		{
			IDXGIOutput* output = nullptr;
			HRESULT hr = aSwapChain->GetContainingOutput(&output);
			if (FAILED(hr) || !output)
				return false;

			IDXGIOutput6* output6 = nullptr;
			hr = output->QueryInterface(__uuidof(IDXGIOutput6), reinterpret_cast<void**>(&output6));
			output->Release();

			if (FAILED(hr) || !output6)
				return false;

			DXGI_OUTPUT_DESC1 desc1{};
			hr = output6->GetDesc1(&desc1);
			output6->Release();

			if (FAILED(hr))
				return false;

			return (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ||
			        desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
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
