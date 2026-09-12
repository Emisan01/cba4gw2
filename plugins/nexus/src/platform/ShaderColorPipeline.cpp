#include "ShaderColorPipeline.h"

#include <d3dcompiler.h>
#include <cstring>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace cba
{
	namespace
	{
		// One fullscreen triangle generated from SV_VertexID - no vertex
		// buffer, no input layout, nothing to bind or restore. The classic
		// three-vertex trick: it covers the viewport with a single primitive,
		// which also avoids the diagonal seam two triangles produce on some
		// drivers.
		const char* kVertexShaderHlsl = R"(
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
VSOut main(uint vid : SV_VertexID)
{
    VSOut o;
    o.uv  = float2((vid << 1) & 2, vid & 2);
    o.pos = float4(o.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}
)";

		// The same 3x3 the DWM path fed to MagSetFullscreenColorEffect, applied
		// per pixel instead of per desktop. Alpha is passed through untouched:
		// we are correcting colour, not compositing.
		//
		// saturate() rather than a soft rolloff on purpose - it matches
		// ColorMatrix::ApplyPixel's clamp, so the shader and the CPU-side maths
		// the unit tests pin agree at the boundaries too.
		const char* kPixelShaderHlsl = R"(
Texture2D    SrcTex : register(t0);
SamplerState SrcSmp : register(s0);

cbuffer Params : register(b0)
{
    float4 Row0;
    float4 Row1;
    float4 Row2;
};

struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float4 main(VSOut i) : SV_Target
{
    float4 c = SrcTex.Sample(SrcSmp, i.uv);
    float3 o;
    o.r = dot(c.rgb, Row0.rgb);
    o.g = dot(c.rgb, Row1.rgb);
    o.b = dot(c.rgb, Row2.rgb);
    return float4(saturate(o), c.a);
}
)";

		struct MatrixCB
		{
			float row0[4];
			float row1[4];
			float row2[4];
		};

		// Everything this pass touches on the device context, so it can be put
		// back exactly as found. The context belongs to the game and to Nexus;
		// we are a guest in the middle of their frame. ImGui's own DX11 backend
		// does the same thing for the same reason - it is the convention here,
		// not paranoia.
		struct StateBackup
		{
			ID3D11RenderTargetView*   rtv = nullptr;
			ID3D11DepthStencilView*   dsv = nullptr;
			ID3D11BlendState*         blend = nullptr;
			FLOAT                     blendFactor[4]{};
			UINT                      sampleMask = 0;
			ID3D11DepthStencilState*  depth = nullptr;
			UINT                      stencilRef = 0;
			ID3D11RasterizerState*    raster = nullptr;
			D3D11_VIEWPORT            viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
			UINT                      viewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
			ID3D11ShaderResourceView* srv0 = nullptr;
			ID3D11SamplerState*       samp0 = nullptr;
			ID3D11Buffer*             cb0 = nullptr;
			ID3D11VertexShader*       vs = nullptr;
			ID3D11PixelShader*        ps = nullptr;
			ID3D11GeometryShader*     gs = nullptr;
			ID3D11InputLayout*        layout = nullptr;
			D3D11_PRIMITIVE_TOPOLOGY  topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		};

		void Capture(ID3D11DeviceContext* aCtx, StateBackup& aOut)
		{
			aCtx->OMGetRenderTargets(1, &aOut.rtv, &aOut.dsv);
			aCtx->OMGetBlendState(&aOut.blend, aOut.blendFactor, &aOut.sampleMask);
			aCtx->OMGetDepthStencilState(&aOut.depth, &aOut.stencilRef);
			aCtx->RSGetState(&aOut.raster);
			aCtx->RSGetViewports(&aOut.viewportCount, aOut.viewports);
			aCtx->PSGetShaderResources(0, 1, &aOut.srv0);
			aCtx->PSGetSamplers(0, 1, &aOut.samp0);
			aCtx->PSGetConstantBuffers(0, 1, &aOut.cb0);
			aCtx->VSGetShader(&aOut.vs, nullptr, nullptr);
			aCtx->PSGetShader(&aOut.ps, nullptr, nullptr);
			aCtx->GSGetShader(&aOut.gs, nullptr, nullptr);
			aCtx->IAGetInputLayout(&aOut.layout);
			aCtx->IAGetPrimitiveTopology(&aOut.topology);
		}

		void Restore(ID3D11DeviceContext* aCtx, StateBackup& aIn)
		{
			aCtx->OMSetRenderTargets(1, &aIn.rtv, aIn.dsv);
			aCtx->OMSetBlendState(aIn.blend, aIn.blendFactor, aIn.sampleMask);
			aCtx->OMSetDepthStencilState(aIn.depth, aIn.stencilRef);
			aCtx->RSSetState(aIn.raster);
			if (aIn.viewportCount) aCtx->RSSetViewports(aIn.viewportCount, aIn.viewports);
			aCtx->PSSetShaderResources(0, 1, &aIn.srv0);
			aCtx->PSSetSamplers(0, 1, &aIn.samp0);
			aCtx->PSSetConstantBuffers(0, 1, &aIn.cb0);
			aCtx->VSSetShader(aIn.vs, nullptr, 0);
			aCtx->PSSetShader(aIn.ps, nullptr, 0);
			aCtx->GSSetShader(aIn.gs, nullptr, 0);
			aCtx->IASetInputLayout(aIn.layout);
			aCtx->IASetPrimitiveTopology(aIn.topology);

			if (aIn.rtv)    aIn.rtv->Release();
			if (aIn.dsv)    aIn.dsv->Release();
			if (aIn.blend)  aIn.blend->Release();
			if (aIn.depth)  aIn.depth->Release();
			if (aIn.raster) aIn.raster->Release();
			if (aIn.srv0)   aIn.srv0->Release();
			if (aIn.samp0)  aIn.samp0->Release();
			if (aIn.cb0)    aIn.cb0->Release();
			if (aIn.vs)     aIn.vs->Release();
			if (aIn.ps)     aIn.ps->Release();
			if (aIn.gs)     aIn.gs->Release();
			if (aIn.layout) aIn.layout->Release();
		}

		template <typename T>
		void SafeRelease(T*& aPtr)
		{
			if (aPtr) { aPtr->Release(); aPtr = nullptr; }
		}
	}

	bool ShaderColorPipeline::Initialize(IDXGISwapChain* aSwapChain)
	{
		if (_ready) return true;
		if (!aSwapChain) { _lastError = "No swapchain"; return false; }

		if (FAILED(aSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&_device)) || !_device)
		{
			_lastError = "GetDevice failed";
			return false;
		}
		_device->GetImmediateContext(&_context);
		if (!_context) { _lastError = "No immediate context"; Shutdown(); return false; }

		// Compiled at load, once. d3dcompiler_47.dll ships with Windows 10+
		// and is already in this process (the D3D11 runtime pulls it), so this
		// adds no deployment dependency.
		ID3DBlob* vsBlob = nullptr;
		ID3DBlob* psBlob = nullptr;
		ID3DBlob* errBlob = nullptr;

		if (FAILED(D3DCompile(kVertexShaderHlsl, std::strlen(kVertexShaderHlsl), "cba_vs", nullptr, nullptr,
			"main", "vs_4_0", 0, 0, &vsBlob, &errBlob)))
		{
			_lastError = "Vertex shader compile failed";
			SafeRelease(errBlob);
			Shutdown();
			return false;
		}
		SafeRelease(errBlob);

		if (FAILED(D3DCompile(kPixelShaderHlsl, std::strlen(kPixelShaderHlsl), "cba_ps", nullptr, nullptr,
			"main", "ps_4_0", 0, 0, &psBlob, &errBlob)))
		{
			_lastError = "Pixel shader compile failed";
			SafeRelease(errBlob);
			SafeRelease(vsBlob);
			Shutdown();
			return false;
		}
		SafeRelease(errBlob);

		HRESULT hr = _device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_vs);
		if (SUCCEEDED(hr))
			hr = _device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_ps);
		SafeRelease(vsBlob);
		SafeRelease(psBlob);
		if (FAILED(hr)) { _lastError = "Shader object creation failed"; Shutdown(); return false; }

		D3D11_BUFFER_DESC cbd{};
		cbd.ByteWidth = sizeof(MatrixCB);
		cbd.Usage = D3D11_USAGE_DYNAMIC;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (FAILED(_device->CreateBuffer(&cbd, nullptr, &_cb)))
		{
			_lastError = "Constant buffer creation failed";
			Shutdown();
			return false;
		}

		// Opaque overwrite, no depth, no culling. The source is a full-screen
		// copy of the frame we are replacing, so there is nothing to blend with
		// and nothing to occlude.
		D3D11_BLEND_DESC bd{};
		bd.RenderTarget[0].BlendEnable = FALSE;
		bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		if (FAILED(_device->CreateBlendState(&bd, &_blend)))
		{
			_lastError = "Blend state creation failed";
			Shutdown();
			return false;
		}

		D3D11_RASTERIZER_DESC rd{};
		rd.FillMode = D3D11_FILL_SOLID;
		rd.CullMode = D3D11_CULL_NONE;
		rd.DepthClipEnable = FALSE;
		rd.ScissorEnable = FALSE;
		if (FAILED(_device->CreateRasterizerState(&rd, &_raster)))
		{
			_lastError = "Rasterizer state creation failed";
			Shutdown();
			return false;
		}

		D3D11_DEPTH_STENCIL_DESC dsd{};
		dsd.DepthEnable = FALSE;
		dsd.StencilEnable = FALSE;
		if (FAILED(_device->CreateDepthStencilState(&dsd, &_depth)))
		{
			_lastError = "Depth-stencil state creation failed";
			Shutdown();
			return false;
		}

		// Point sampling, clamped. The source is the same resolution as the
		// target and mapped 1:1, so any filtering would only ever soften a
		// pixel-exact copy - and softening is the opposite of what a tool for
		// telling colours apart should do.
		D3D11_SAMPLER_DESC sd{};
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sd.MaxLOD = D3D11_FLOAT32_MAX;
		if (FAILED(_device->CreateSamplerState(&sd, &_sampler)))
		{
			_lastError = "Sampler creation failed";
			Shutdown();
			return false;
		}

		_lastError = "";
		_ready = true;
		return true;
	}

	void ShaderColorPipeline::ReleaseRenderTarget()
	{
		SafeRelease(_rtv);
		_rtvSource = nullptr;
	}

	bool ShaderColorPipeline::EnsureRenderTarget(IDXGISwapChain* aSwapChain)
	{
		// Built fresh every frame, on purpose, after a first version cached it.
		//
		// Caching needs a way to know the backbuffer was swapped out from under
		// us. Nexus can cache its own RTV because it hooks ResizeBuffers and
		// rebuilds on that signal; we get no such notification. The tempting
		// substitute - remembering the backbuffer pointer and comparing - is
		// unsound: once released, that address can be handed back out for a
		// different object, so a match proves nothing and OMSetRenderTargets
		// would be pointing at freed memory. In Emi's game. Not a trade worth
		// making to save one lightweight object creation per frame.
		ReleaseRenderTarget();

		ID3D11Texture2D* backBuffer = nullptr;
		if (FAILED(aSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer)) || !backBuffer)
			return false;

		HRESULT hr = _device->CreateRenderTargetView(backBuffer, nullptr, &_rtv);
		backBuffer->Release();
		return SUCCEEDED(hr);
	}

	void ShaderColorPipeline::ReleaseSourceTexture()
	{
		SafeRelease(_srcSrv);
		SafeRelease(_srcTex);
		_srcW = 0;
		_srcH = 0;
		_srcFormat = DXGI_FORMAT_UNKNOWN;
	}

	bool ShaderColorPipeline::EnsureSourceTexture(ID3D11Texture2D* aBackBuffer)
	{
		D3D11_TEXTURE2D_DESC bd{};
		aBackBuffer->GetDesc(&bd);

		// A multisampled backbuffer cannot be CopyResource'd into a
		// single-sample texture - the call fails and returns nothing, so the
		// shader would sample a black frame and the screen would go dark with
		// no error anywhere. Refusing loudly is the only honest option; every
		// swapchain backbuffer seen in practice is single-sample, so this is a
		// guard against a silent failure mode, not an expected path.
		if (bd.SampleDesc.Count != 1)
		{
			_lastError = "Backbuffer is multisampled - shader path not supported here";
			return false;
		}

		if (_srcTex && _srcW == bd.Width && _srcH == bd.Height && _srcFormat == bd.Format)
			return true;

		ReleaseSourceTexture();

		D3D11_TEXTURE2D_DESC td{};
		td.Width = bd.Width;
		td.Height = bd.Height;
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.Format = bd.Format;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		if (FAILED(_device->CreateTexture2D(&td, nullptr, &_srcTex)))
		{
			_lastError = "Source texture creation failed";
			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
		sd.Format = bd.Format;
		sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		sd.Texture2D.MipLevels = 1;
		if (FAILED(_device->CreateShaderResourceView(_srcTex, &sd, &_srcSrv)))
		{
			_lastError = "Source SRV creation failed";
			ReleaseSourceTexture();
			return false;
		}

		_srcW = bd.Width;
		_srcH = bd.Height;
		_srcFormat = bd.Format;
		_lastError = "";
		return true;
	}

	void ShaderColorPipeline::Apply(IDXGISwapChain* aSwapChain, const double aMatrix[3][3])
	{
		if (!_ready || !aSwapChain || !_context) return;
		if (!EnsureRenderTarget(aSwapChain)) return;

		ID3D11Texture2D* backBuffer = nullptr;
		if (FAILED(aSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer)) || !backBuffer)
			return;

		if (!EnsureSourceTexture(backBuffer))
		{
			backBuffer->Release();
			return;
		}

		// The frame as the game rendered it, on its way to becoming the shader
		// input. This copy is the one real cost of the whole approach: a
		// full-resolution GPU-to-GPU blit per frame. It is also strictly less
		// than what HybridScanner already pays on its scan ticks, because that
		// one additionally Maps the result for CPU reading, which stalls the
		// pipeline. This does not leave the GPU.
		_context->CopyResource(_srcTex, backBuffer);
		backBuffer->Release();

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(_context->Map(_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
		MatrixCB* cb = static_cast<MatrixCB*>(mapped.pData);
		for (int row = 0; row < 3; ++row)
		{
			float* dst = (row == 0) ? cb->row0 : (row == 1) ? cb->row1 : cb->row2;
			dst[0] = static_cast<float>(aMatrix[row][0]);
			dst[1] = static_cast<float>(aMatrix[row][1]);
			dst[2] = static_cast<float>(aMatrix[row][2]);
			dst[3] = 0.0f;
		}
		_context->Unmap(_cb, 0);

		StateBackup backup{};
		Capture(_context, backup);

		D3D11_VIEWPORT vp{};
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = static_cast<float>(_srcW);
		vp.Height = static_cast<float>(_srcH);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		const FLOAT blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		_context->OMSetRenderTargets(1, &_rtv, nullptr);
		_context->RSSetViewports(1, &vp);
		_context->RSSetState(_raster);
		_context->OMSetBlendState(_blend, blendFactor, 0xFFFFFFFF);
		_context->OMSetDepthStencilState(_depth, 0);
		_context->IASetInputLayout(nullptr);
		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_context->VSSetShader(_vs, nullptr, 0);
		_context->PSSetShader(_ps, nullptr, 0);
		_context->GSSetShader(nullptr, nullptr, 0);
		_context->PSSetShaderResources(0, 1, &_srcSrv);
		_context->PSSetSamplers(0, 1, &_sampler);
		_context->PSSetConstantBuffers(0, 1, &_cb);

		_context->Draw(3, 0);

		// Unbind our SRV before handing the context back: leaving a texture
		// bound that is about to be a copy destination again next frame is how
		// you earn a D3D11 warning storm and, on some drivers, a stall.
		ID3D11ShaderResourceView* nullSrv = nullptr;
		_context->PSSetShaderResources(0, 1, &nullSrv);

		Restore(_context, backup);
	}

	void ShaderColorPipeline::Shutdown()
	{
		ReleaseRenderTarget();
		ReleaseSourceTexture();
		SafeRelease(_sampler);
		SafeRelease(_depth);
		SafeRelease(_raster);
		SafeRelease(_blend);
		SafeRelease(_cb);
		SafeRelease(_ps);
		SafeRelease(_vs);
		SafeRelease(_context);
		SafeRelease(_device);
		_ready = false;
	}

	ShaderColorPipeline& GetShaderColorPipeline()
	{
		static ShaderColorPipeline s_pipeline;
		return s_pipeline;
	}
}
