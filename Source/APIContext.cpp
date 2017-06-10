#include "stdafx.h"
#include "APIContext.h"


namespace zeta
{
	typedef HRESULT(WINAPI *CreateDXGIFactory1Func)(REFIID riid, void** ppFactory);
	typedef HRESULT(WINAPI *D3D11CreateDeviceFunc)(IDXGIAdapter* pAdapter,
		D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags,
		D3D_FEATURE_LEVEL const * pFeatureLevels, UINT FeatureLevels, UINT SDKVersion,
		ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext);

	bool DynamicFuncInit_ = false;
	CreateDXGIFactory1Func DynamicCreateDXGIFactory1_ = nullptr;
	D3D11CreateDeviceFunc DynamicD3D11CreateDevice_ = nullptr;
	pD3DCompile DynamicD3DCompile_ = nullptr;
	HMODULE ModD3D11_ = nullptr;
	HMODULE ModDXGI_ = nullptr;
	HMODULE ModD3DCompiler_ = nullptr;

	APIContext::APIContext()
	{
		wnd_ = nullptr;

		if (!DynamicFuncInit_)
		{
			DynamicFuncInit_ = true;

			ModDXGI_ = ::LoadLibraryEx(TEXT("dxgi.dll"), nullptr, 0);
			if (nullptr == ModDXGI_)
			{
				::MessageBoxW(nullptr, L"Can't load dxgi.dll", L"Error", MB_OK);
			}
			ModD3D11_ = ::LoadLibraryEx(TEXT("d3d11.dll"), nullptr, 0);
			if (nullptr == ModD3D11_)
			{
				::MessageBoxW(nullptr, L"Can't load d3d11.dll", L"Error", MB_OK);
			}
			ModD3DCompiler_ = ::LoadLibraryEx(TEXT("d3dcompiler_47.dll"), nullptr, 0);
			if (nullptr == ModD3DCompiler_)
			{
				::MessageBoxW(nullptr, L"Can't load d3dcompiler_47.dll", L"Error", MB_OK);
			}

			if (ModDXGI_ != nullptr)
			{
				DynamicCreateDXGIFactory1_ = reinterpret_cast<CreateDXGIFactory1Func>(::GetProcAddress(ModDXGI_, "CreateDXGIFactory1"));
			}
			if (ModD3D11_ != nullptr)
			{
				DynamicD3D11CreateDevice_ = reinterpret_cast<D3D11CreateDeviceFunc>(::GetProcAddress(ModD3D11_, "D3D11CreateDevice"));
			}
			if (ModD3DCompiler_ != nullptr)
			{
				DynamicD3DCompile_ = reinterpret_cast<pD3DCompile>(::GetProcAddress(ModD3DCompiler_, "D3DCompile"));
			}
		}
	}

	APIContext::~APIContext()
	{
		if (gi_swap_chain_1_)
		{
			gi_swap_chain_1_->SetFullscreenState(false, nullptr);
		}

		effects_.clear();

		d3d_imm_ctx_.reset();
		d3d_device_.reset();

		gi_swap_chain_1_.reset();
		gi_adapter_.reset();
		gi_factory_1_.reset();
		gi_factory_2_.reset();
	}

	APIContext& APIContext::Instance()
	{
		static APIContext obj;
		return obj;
	}

	void APIContext::Create(HWND wnd)
	{
		wnd_ = wnd;

		IDXGIFactory1* gi_factory;
		THROW_FAILED(DynamicCreateDXGIFactory1_(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&gi_factory)));
		gi_factory_1_ = MakeCOMPtr(gi_factory);

		IDXGIFactory2* gi_factory2;
		THROW_FAILED(gi_factory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&gi_factory2)));
		gi_factory_2_ = MakeCOMPtr(gi_factory2);

		IDXGIAdapter* dxgi_adapter = nullptr;
		THROW_FAILED(gi_factory_1_->EnumAdapters(0, &dxgi_adapter));
		gi_adapter_ = MakeCOMPtr(dxgi_adapter);

		IDXGIOutput* output = nullptr;
		THROW_FAILED(dxgi_adapter->EnumOutputs(0, &output));

		UINT num = 0;
		THROW_FAILED(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &num, 0));

		std::vector<DXGI_MODE_DESC> mode_descs(num);
		THROW_FAILED(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &num, &mode_descs[0]));

		output->Release();
		output = nullptr;

		std::vector<D3D_FEATURE_LEVEL> d3d_feature_levels;
		d3d_feature_levels.emplace_back(D3D_FEATURE_LEVEL_11_1);
		d3d_feature_levels.emplace_back(D3D_FEATURE_LEVEL_11_0);
		d3d_feature_levels.emplace_back(D3D_FEATURE_LEVEL_10_1);
		d3d_feature_levels.emplace_back(D3D_FEATURE_LEVEL_10_0);
		d3d_feature_levels.emplace_back(D3D_FEATURE_LEVEL_9_3);

		D3D_FEATURE_LEVEL d3d_out_feature_level = D3D_FEATURE_LEVEL_9_3;

		//Device and context
		ID3D11Device* d3d_device = nullptr;
		ID3D11DeviceContext* d3d_imm_ctx = nullptr;

		THROW_FAILED(DynamicD3D11CreateDevice_(dxgi_adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
			d3d_feature_levels.data(), (UINT)d3d_feature_levels.size(), D3D11_SDK_VERSION, &d3d_device,
			&d3d_out_feature_level, &d3d_imm_ctx));
		d3d_device_ = MakeCOMPtr(d3d_device);
		d3d_imm_ctx_ = MakeCOMPtr(d3d_imm_ctx);
	}

	void APIContext::ResizeSwapChain(uint32_t width, uint32_t height)
	{
		if (!wnd_)
		{
			return;
		}

		IDXGISwapChain1* dxgi_sc = nullptr;
		if (gi_swap_chain_1_)
		{
			dxgi_sc = gi_swap_chain_1_.get();
			THROW_FAILED(dxgi_sc->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
				DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
		}
		else
		{
			DXGI_SWAP_CHAIN_DESC1 sc_desc1;
			ZeroMemory(&sc_desc1, sizeof(sc_desc1));
			sc_desc1.Width = width;
			sc_desc1.Height = height;
			sc_desc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sc_desc1.Stereo = false;
			sc_desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sc_desc1.BufferCount = 2;
			sc_desc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
			sc_desc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			sc_desc1.SampleDesc.Count = 1;
			sc_desc1.SampleDesc.Quality = 0;
			sc_desc1.Scaling = DXGI_SCALING_STRETCH;
			sc_desc1.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC sc_fs_desc;
			sc_fs_desc.RefreshRate.Numerator = 60;
			sc_fs_desc.RefreshRate.Denominator = 1;
			sc_fs_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			sc_fs_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			sc_fs_desc.Windowed = true;

			THROW_FAILED(gi_factory_2_->CreateSwapChainForHwnd(d3d_device_.get(), wnd_, &sc_desc1, &sc_fs_desc, nullptr, &dxgi_sc));
			gi_swap_chain_1_ = MakeCOMPtr(dxgi_sc);
		}
	}

	HWND APIContext::Wnd()
	{
		return wnd_;
	}

	ID3DX11Effect* APIContext::GetEffect(std::string file_path)
	{
		auto pos = effects_.find(file_path);
		if (pos != effects_.end())
		{
			return pos->second.get();
		}

		std::wstring wfile_path = ToW(file_path);

		ID3DX11Effect* d3d_effect = nullptr;
		ID3DBlob* d3d_error_blob = nullptr;

		HRESULT hr = D3DX11CompileEffectFromFile(wfile_path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			D3DCOMPILE_ENABLE_STRICTNESS, 0, this->D3DDevice(), &d3d_effect, &d3d_error_blob);

		if (FAILED(hr) && d3d_error_blob)
		{
			const char* err = reinterpret_cast<const char*>(d3d_error_blob->GetBufferPointer());
			std::string serr = err;

			d3d_error_blob->Release();
			d3d_error_blob = nullptr;

			DO_THROW_MSG(serr.c_str());
		}
		THROW_FAILED(hr);

		effects_[file_path] = MakeCOMPtr(d3d_effect);
		return d3d_effect;
	}

	IDXGISwapChain1* APIContext::DXGISwapChain()
	{
		return gi_swap_chain_1_.get();
	}

	ID3D11Device* APIContext::D3DDevice()
	{
		return d3d_device_.get();
	}

	ID3D11DeviceContext* APIContext::D3DImmContext()
	{
		return d3d_imm_ctx_.get();
	}

	HRESULT APIContext::D3DCompile(std::string const & src_data, const char* entry_point, const char* target,
		std::vector<uint8_t>& code, std::string& error_msgs) const
	{
		ID3DBlob* code_blob = nullptr;
		ID3DBlob* error_msgs_blob = nullptr;

		uint32_t const flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_SKIP_OPTIMIZATION;
		HRESULT hr = DynamicD3DCompile_(src_data.c_str(), static_cast<UINT>(src_data.size()),
			nullptr, nullptr, nullptr, entry_point,
			target, flags, 0, &code_blob, &error_msgs_blob);
		if (code_blob)
		{
			uint8_t const * p = static_cast<uint8_t const *>(code_blob->GetBufferPointer());
			code.assign(p, p + code_blob->GetBufferSize());
			code_blob->Release();
		}
		else
		{
			code.clear();
		}
		if (error_msgs_blob)
		{
			char const * p = static_cast<char const *>(error_msgs_blob->GetBufferPointer());
			error_msgs.assign(p, p + error_msgs_blob->GetBufferSize());
			error_msgs_blob->Release();
		}
		else
		{
			error_msgs.clear();
		}
		return hr;
	}

	ID3D11Texture2D* APIContext::D3DCreateTexture2D(UINT width, UINT height, DXGI_FORMAT fmt, UINT bind_flags)
	{
		D3D11_TEXTURE2D_DESC d3d_tex_desc;
		ZeroMemory(&d3d_tex_desc, sizeof(d3d_tex_desc));
		d3d_tex_desc.Width = width;
		d3d_tex_desc.Height = height;
		d3d_tex_desc.MipLevels = 1;
		d3d_tex_desc.ArraySize = 1;
		d3d_tex_desc.Format = (DXGI_FORMAT)fmt;
		d3d_tex_desc.SampleDesc.Count = 1;
		d3d_tex_desc.SampleDesc.Quality = 0;
		d3d_tex_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_tex_desc.BindFlags = bind_flags;
		d3d_tex_desc.CPUAccessFlags = 0;
		d3d_tex_desc.MiscFlags = 0;

		ID3D11Texture2D* d3d_tex = nullptr;
		THROW_FAILED(d3d_device_->CreateTexture2D(&d3d_tex_desc, NULL, &d3d_tex));

		return d3d_tex;
	}

	ID3D11DepthStencilView* APIContext::D3DCreateDepthStencilView(ID3D11Texture2D* tex, DXGI_FORMAT fmt)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC d3d_dsv_desc;
		ZeroMemory(&d3d_dsv_desc, sizeof(d3d_dsv_desc));
		d3d_dsv_desc.Format = (DXGI_FORMAT)fmt;
		d3d_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		d3d_dsv_desc.Texture2D.MipSlice = 0;

		ID3D11DepthStencilView* d3d_dsv = nullptr;
		THROW_FAILED(d3d_device_->CreateDepthStencilView(tex, &d3d_dsv_desc, &d3d_dsv));

		return d3d_dsv;
	}

	ID3D11RenderTargetView* APIContext::D3DCreateRenderTargetView(ID3D11Texture2D* tex)
	{
		ID3D11RenderTargetView* d3d_rtv = nullptr;
		THROW_FAILED(d3d_device_->CreateRenderTargetView(tex, NULL, &d3d_rtv));

		return d3d_rtv;
	}

}