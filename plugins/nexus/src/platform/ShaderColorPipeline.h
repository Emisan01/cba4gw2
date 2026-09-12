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
	// It also runs in ERenderType_PreRender, which Nexus dispatches BEFORE
	// ImGui::NewFrame and before ImGui's draw data reaches the backbuffer
	// (UiContext.cpp:428 vs 491). Consequence worth stating out loud: the
	// correction lands on the game and NOT on CBA's own interface. Under DWM
	// everything was corrected including our own colour swatches and Vision
	// Lab's anomaloscope - i.e. the clinical test was being viewed through the
	// correction it exists to measure. That stops being true here.
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
