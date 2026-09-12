#pragma once

#include <d3d11.h>

namespace cba
{
	// ── What the filter actually did, measured instead of predicted ──────────
	//
	// Everything CBA says about its own effect today is a PREDICTION from the
	// matrix: "Luminanz-Retention 102.8%" is computed from nine reference tag
	// colours, not from the screen. That was the only option while the
	// correction happened inside DWM, where we never saw the result.
	//
	// The shader path changed that. Inside the colour pass we hold both halves
	// of the same frame: the copy taken before the correction, and the
	// backbuffer after it. The difference between them IS the filter's effect,
	// on the actual image, this frame. That is the one thing about its own
	// behaviour this tool can honestly measure.
	//
	// What it deliberately does NOT claim: nothing here knows monitor
	// brightness, panel gamma, HDR tone mapping, or what the eye does with any
	// of it. Those are outside the process. This measures the difference CBA
	// itself generates and stops there - see PRODUCT_CONCEPT.md's rule about
	// supplying evidence rather than claims. A sensor that quietly overstated
	// its reach would be worse than no sensor.
	//
	// How: a full-frame mean via the GPU's own mip chain. Copy mip 0, call
	// GenerateMips, read the 1x1 top - three calls and four bytes back, instead
	// of mapping eight million pixels the way HybridScanner has to. Throttled,
	// and only ever running while a readout is actually on screen.
	class FilterSensor
	{
	public:
		struct Reading
		{
			bool  valid = false;
			float beforeR = 0.0f, beforeG = 0.0f, beforeB = 0.0f;
			float afterR = 0.0f, afterG = 0.0f, afterB = 0.0f;
		};

		// Only true while something is displaying the numbers. The whole cost
		// of this module is conditional on it.
		bool IsEnabled() const { return _enabled; }
		void SetEnabled(bool aEnabled);

		// Called from inside the colour pass, which is the only place both
		// halves of the frame exist at once. Cheap and self-throttling when
		// disabled or called too often.
		void Sample(ID3D11Device* aDevice, ID3D11DeviceContext* aContext,
		            ID3D11Texture2D* aBefore, ID3D11Texture2D* aAfter);

		Reading Latest() const { return _latest; }
		void Shutdown();
		const char* LastError() const { return _lastError; }

	private:
		struct Channel
		{
			ID3D11Texture2D* mips = nullptr;
			ID3D11ShaderResourceView* srv = nullptr;
			ID3D11Texture2D* staging = nullptr;
			UINT topMip = 0;
		};

		bool EnsureChannel(ID3D11Device* aDevice, Channel& aCh, const D3D11_TEXTURE2D_DESC& aSrc);
		void ReleaseChannel(Channel& aCh);
		bool ReadChannel(ID3D11DeviceContext* aContext, Channel& aCh, float& aR, float& aG, float& aB);

		Channel _before{};
		Channel _after{};
		UINT _width = 0;
		UINT _height = 0;
		DXGI_FORMAT _format = DXGI_FORMAT_UNKNOWN;
		bool _enabled = false;
		unsigned long long _lastSampleTick = 0;
		Reading _latest{};
		const char* _lastError = "";
	};

	FilterSensor& GetFilterSensor();

	// BT.709 relative luminance, the same weights the audit pins for the rest
	// of the project (0.2126 / 0.7152 / 0.0722).
	float SensorLuma(float aR, float aG, float aB);
}
