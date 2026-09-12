#include "FilterSensor.h"

#include <windows.h>

namespace cba
{
	namespace
	{
		template <typename T>
		void SafeRelease(T*& aPtr)
		{
			if (aPtr) { aPtr->Release(); aPtr = nullptr; }
		}

		// 5 Hz. A readout a human is looking at does not need per-frame
		// resolution, and the mip generation below is real GPU work on a 4K
		// frame - paying it 100 times a second to update a number nobody can
		// read that fast would be the wrong trade in a tool whose whole point
		// is not disturbing the game.
		constexpr unsigned long long kSampleIntervalMs = 200;
	}

	float SensorLuma(float aR, float aG, float aB)
	{
		return 0.2126f * aR + 0.7152f * aG + 0.0722f * aB;
	}

	void FilterSensor::SetEnabled(bool aEnabled)
	{
		if (_enabled == aEnabled) return;
		_enabled = aEnabled;
		if (!_enabled)
		{
			// Give the memory back rather than holding two mip chains of a 4K
			// frame for a window that is closed. Re-created on the next enable.
			Shutdown();
		}
	}

	void FilterSensor::ReleaseChannel(Channel& aCh)
	{
		SafeRelease(aCh.srv);
		SafeRelease(aCh.staging);
		SafeRelease(aCh.mips);
		aCh.topMip = 0;
	}

	bool FilterSensor::EnsureChannel(ID3D11Device* aDevice, Channel& aCh, const D3D11_TEXTURE2D_DESC& aSrc)
	{
		if (aCh.mips) return true;

		D3D11_TEXTURE2D_DESC td{};
		td.Width = aSrc.Width;
		td.Height = aSrc.Height;
		td.MipLevels = 0; // full chain down to 1x1, D3D fills in the count
		td.ArraySize = 1;
		td.Format = aSrc.Format;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		if (FAILED(aDevice->CreateTexture2D(&td, nullptr, &aCh.mips)))
		{
			_lastError = "Sensor mip texture creation failed";
			return false;
		}

		D3D11_TEXTURE2D_DESC actual{};
		aCh.mips->GetDesc(&actual);
		aCh.topMip = (actual.MipLevels > 0) ? (actual.MipLevels - 1) : 0;

		if (FAILED(aDevice->CreateShaderResourceView(aCh.mips, nullptr, &aCh.srv)))
		{
			_lastError = "Sensor SRV creation failed";
			ReleaseChannel(aCh);
			return false;
		}

		// One pixel is the entire readback. That is the point of doing the
		// averaging on the GPU.
		D3D11_TEXTURE2D_DESC sd{};
		sd.Width = 1;
		sd.Height = 1;
		sd.MipLevels = 1;
		sd.ArraySize = 1;
		sd.Format = aSrc.Format;
		sd.SampleDesc.Count = 1;
		sd.Usage = D3D11_USAGE_STAGING;
		sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		if (FAILED(aDevice->CreateTexture2D(&sd, nullptr, &aCh.staging)))
		{
			_lastError = "Sensor staging texture creation failed";
			ReleaseChannel(aCh);
			return false;
		}
		return true;
	}

	bool FilterSensor::ReadChannel(ID3D11DeviceContext* aContext, Channel& aCh, float& aR, float& aG, float& aB)
	{
		D3D11_MAPPED_SUBRESOURCE mapped{};
		// DO_NOT_WAIT rather than a blocking map: this runs inside the game's
		// render thread. A sensor that stalls the frame it is measuring would
		// be measuring something it caused. If the copy is not finished yet we
		// simply keep the previous reading - at 5 Hz that is invisible.
		const HRESULT hr = aContext->Map(aCh.staging, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped);
		if (hr == DXGI_ERROR_WAS_STILL_DRAWING) return false;
		if (FAILED(hr)) return false;

		const unsigned char* px = static_cast<const unsigned char*>(mapped.pData);
		if (!px)
		{
			aContext->Unmap(aCh.staging, 0);
			return false;
		}

		// Both formats CBA has ever seen on a GW2 backbuffer are 8-bit
		// four-channel; they differ only in whether red or blue comes first.
		const bool bgra = (_format == DXGI_FORMAT_B8G8R8A8_UNORM ||
		                   _format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
		const float c0 = px[0] / 255.0f;
		const float c1 = px[1] / 255.0f;
		const float c2 = px[2] / 255.0f;
		aR = bgra ? c2 : c0;
		aG = c1;
		aB = bgra ? c0 : c2;

		aContext->Unmap(aCh.staging, 0);
		return true;
	}

	void FilterSensor::Sample(ID3D11Device* aDevice, ID3D11DeviceContext* aContext,
	                          ID3D11Texture2D* aBefore, ID3D11Texture2D* aAfter)
	{
		if (!_enabled || !aDevice || !aContext || !aBefore || !aAfter) return;

		const unsigned long long now = GetTickCount64();
		if (now - _lastSampleTick < kSampleIntervalMs) return;
		_lastSampleTick = now;

		D3D11_TEXTURE2D_DESC srcDesc{};
		aBefore->GetDesc(&srcDesc);

		// Only 8-bit four-channel formats are decoded above. Refusing loudly
		// beats reporting a number derived from bytes read in the wrong order.
		if (srcDesc.Format != DXGI_FORMAT_R8G8B8A8_UNORM &&
			srcDesc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB &&
			srcDesc.Format != DXGI_FORMAT_B8G8R8A8_UNORM &&
			srcDesc.Format != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
		{
			_lastError = "Sensor supports 8-bit RGBA/BGRA backbuffers only";
			_latest.valid = false;
			return;
		}

		if (_width != srcDesc.Width || _height != srcDesc.Height || _format != srcDesc.Format)
		{
			ReleaseChannel(_before);
			ReleaseChannel(_after);
			_width = srcDesc.Width;
			_height = srcDesc.Height;
			_format = srcDesc.Format;
		}

		if (!EnsureChannel(aDevice, _before, srcDesc)) return;
		if (!EnsureChannel(aDevice, _after, srcDesc)) return;

		// Read what the PREVIOUS tick copied, then queue this tick's copy. One
		// sample of latency, and never a wait on work submitted moments ago.
		float br = 0.0f, bg = 0.0f, bb = 0.0f;
		float ar = 0.0f, ag = 0.0f, ab = 0.0f;
		const bool gotBefore = ReadChannel(aContext, _before, br, bg, bb);
		const bool gotAfter = ReadChannel(aContext, _after, ar, ag, ab);
		if (gotBefore && gotAfter)
		{
			_latest.beforeR = br; _latest.beforeG = bg; _latest.beforeB = bb;
			_latest.afterR = ar;  _latest.afterG = ag;  _latest.afterB = ab;
			_latest.valid = true;
			_lastError = "";
		}

		auto queue = [&](Channel& ch, ID3D11Texture2D* src)
		{
			// Mip 0 only - CopyResource would need identical descriptions, and
			// ours differ precisely because we asked for a mip chain.
			aContext->CopySubresourceRegion(ch.mips, 0, 0, 0, 0, src, 0, nullptr);
			aContext->GenerateMips(ch.srv);
			// The 1x1 top of the chain is the box-filtered mean of the frame.
			// Not a mathematically exact average on a non-power-of-two frame -
			// each halving rounds - but the bias is identical for both halves,
			// and every number this sensor reports is a DIFFERENCE between
			// them, so it cancels where it matters.
			aContext->CopySubresourceRegion(ch.staging, 0, 0, 0, 0, ch.mips, ch.topMip, nullptr);
		};
		queue(_before, aBefore);
		queue(_after, aAfter);
	}

	void FilterSensor::Shutdown()
	{
		ReleaseChannel(_before);
		ReleaseChannel(_after);
		_width = 0;
		_height = 0;
		_format = DXGI_FORMAT_UNKNOWN;
		_latest = Reading{};
	}

	FilterSensor& GetFilterSensor()
	{
		static FilterSensor s_sensor;
		return s_sensor;
	}
}
