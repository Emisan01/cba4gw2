#pragma once

#include <d3d11.h>

namespace cba
{
	// ── The colour correction, applied to GW2's frame instead of the desktop ──
	//
	// This is the replacement for the Magnification / DWM path, not an addition
	// to it. Same maths (EffectiveDisplayMatrix), different delivery: one
	// fullscreen triangle with a pixel shader that multiplies the already
	// rendered frame by the 3x3 matrix, drawn straight into GW2's backbuffer.
	//
	// Why this is not a new kind of access (checked in Emi's Nexus checkout on
	// 2026-09-12, because his account is not something to be casual about):
	//   - Nexus already installs the only hook involved - a MinHook vtable
	//     detour on IDXGISwapChain::Present, Core/Hooks/Hooks.cpp:81.
	//   - Nexus already binds the game's backbuffer as a render target and
	//     draws ImGui into it, UI/UiContext.cpp:348/490. Every CBA window is
	//     already pixels in GW2's frame.
	//   - CBA already takes the D3D11 device off the swapchain, creates its own
	//     textures, CopyResource's the rendered frame and Maps it for reading
	//     (HybridScanner.cpp:489-608), and already draws a display-sized
	//     textured quad over the game (ModuleMain.cpp:1159).
	// So this adds one draw call and a compiled shader. No new hook, no new
	// module, no process or memory access of any kind. It is strictly less
	// machinery than the readback path that has been shipping for weeks.
	//
	// It runs in ERenderType_PostRender (UiContext.cpp:495), still inside the
	// Present detour and so still before the frame is shown. That slot is
	// deliberate: the pass sees game plus tag overlay plus UI - exactly the
	// content the DWM effect saw - which keeps the enhancer's invariant
	// "everything on screen gets M" true and makes this a drop-in replacement
	// rather than a change of semantics.
	//
	// It sat in PreRender for one build. That corrected the game before ImGui,
	// which left CBA's own interface true colour - genuinely desirable, since
	// under DWM even Vision Lab's anomaloscope was viewed through the
	// correction it exists to measure. It also silently broke two things:
	// HybridScanner::ScanFrame (ERenderType_Render, i.e. later) would have
	// matched an already-corrected frame against raw reference tag colours,
	// and the tag overlay - also drawn in Render - would never have received
	// M while the enhancer chose its colours assuming it would. Getting the
	// true-colour UI back properly means giving the enhancer two matrices,
	// one for game pixels and one for overlay pixels; that is a colour-science
	// change and belongs in its own step, not inside a backend swap.
	class ShaderColorPipeline
	{
	public:
		// Builds device resources from the swapchain Nexus hands us. Safe to
		// call repeatedly; only the first successful call does work.
		bool Initialize(IDXGISwapChain* aSwapChain);

		// Releases everything. Safe on a partially initialized pipeline.
		void Shutdown();

		bool IsReady() const { return _ready; }

		// Draws the correction over the current backbuffer contents.
		// aMatrix is row-major [3][3], the same shape ColorMatrix produces.
		// Does nothing if not ready. Saves and restores every piece of device
		// state it touches - this context belongs to the game and to Nexus.
		void Apply(IDXGISwapChain* aSwapChain, const double aMatrix[3][3]);

		// The last compile/creation error, for the diagnostics report. Empty
		// while healthy.
		const char* LastError() const { return _lastError; }

		// The backbuffer format the pass is actually operating on. Reported
		// in diagnostics because Emi runs HDR10, and the maths assumes
		// normalised 0..1 values: 8-bit and 10-bit UNORM are fine, a float
		// (scRGB) backbuffer would carry values above 1.0 that the shader's
		// saturate() would clip. Knowing which one it is beats reasoning
		// about it. DXGI_FORMAT_UNKNOWN until the first frame.
		DXGI_FORMAT SourceFormat() const { return _srcFormat; }

	private:
		bool EnsureRenderTarget(IDXGISwapChain* aSwapChain);
		void ReleaseRenderTarget();
		// A pixel shader cannot read the render target it writes to, so the
		// frame is copied to a shader-readable texture first. Allocated once
		// and reused; only a resolution or format change reallocates.
		bool EnsureSourceTexture(ID3D11Texture2D* aBackBuffer);
		void ReleaseSourceTexture();

		ID3D11Device*           _device      = nullptr;  // borrowed, not owned
		ID3D11DeviceContext*    _context     = nullptr;  // borrowed, not owned
		ID3D11VertexShader*     _vs          = nullptr;
		ID3D11PixelShader*      _ps          = nullptr;
		ID3D11Buffer*           _cb          = nullptr;
		ID3D11BlendState*       _blend       = nullptr;
		ID3D11RasterizerState*  _raster      = nullptr;
		ID3D11DepthStencilState* _depth      = nullptr;
		ID3D11SamplerState*     _sampler     = nullptr;
		ID3D11RenderTargetView* _rtv         = nullptr;
		ID3D11Texture2D*        _rtvSource   = nullptr;  // which backbuffer _rtv belongs to
		ID3D11Texture2D*        _srcTex      = nullptr;
		ID3D11ShaderResourceView* _srcSrv    = nullptr;
		UINT                    _srcW        = 0;
		UINT                    _srcH        = 0;
		DXGI_FORMAT             _srcFormat   = DXGI_FORMAT_UNKNOWN;
		bool                    _ready       = false;
		const char*             _lastError   = "";
	};

	ShaderColorPipeline& GetShaderColorPipeline();
}
