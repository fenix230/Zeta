#include "stdafx.h"
#include "Renderer.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Renderable.h"
#include "Light.h"


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


	Renderer::Renderer()
	{
		wnd_ = nullptr;
		width_ = 0;
		height_ = 0;

		if (!DynamicFuncInit_)
		{
			DynamicFuncInit_ = true;

			mod_dxgi_ = ::LoadLibraryEx(TEXT("dxgi.dll"), nullptr, 0);
			if (nullptr == mod_dxgi_)
			{
				::MessageBoxW(nullptr, L"Can't load dxgi.dll", L"Error", MB_OK);
			}
			mod_d3d11_ = ::LoadLibraryEx(TEXT("d3d11.dll"), nullptr, 0);
			if (nullptr == mod_d3d11_)
			{
				::MessageBoxW(nullptr, L"Can't load d3d11.dll", L"Error", MB_OK);
			}
			mod_d3dcompiler_ = ::LoadLibraryEx(TEXT("d3dcompiler_47.dll"), nullptr, 0);
			if (nullptr == mod_d3dcompiler_)
			{
				::MessageBoxW(nullptr, L"Can't load d3dcompiler_47.dll", L"Error", MB_OK);
			}

			if (mod_dxgi_ != nullptr)
			{
				DynamicCreateDXGIFactory1_ = reinterpret_cast<CreateDXGIFactory1Func>(::GetProcAddress(mod_dxgi_, "CreateDXGIFactory1"));
			}
			if (mod_d3d11_ != nullptr)
			{
				DynamicD3D11CreateDevice_ = reinterpret_cast<D3D11CreateDeviceFunc>(::GetProcAddress(mod_d3d11_, "D3D11CreateDevice"));
			}
			if (mod_d3dcompiler_ != nullptr)
			{
				DynamicD3DCompile_ = reinterpret_cast<pD3DCompile>(::GetProcAddress(mod_d3dcompiler_, "D3DCompile"));
			}
		}
	}

	Renderer::~Renderer()
	{
		this->Destory();
	}

	Renderer& Renderer::Instance()
	{
		static Renderer obj;
		return obj;
	}

	void Renderer::Create(HWND wnd, int width, int height)
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

		quad_ = std::make_shared<QuadRenderable>();

		this->Resize(width, height);

		this->LoadEffect("Shader/DeferredRendering.fx");
	}

	void Renderer::Resize(int width, int height)
	{
		width_ = width;
		height_ = height;

		d3d_imm_ctx_->OMSetRenderTargets(0, 0, 0);
		d3d_imm_ctx_->OMSetDepthStencilState(0, 0);

		gbuffer_fb_.reset();
		lighting_fb_.reset();
		srgb_fb_.reset();

		//SwapChain
		IDXGISwapChain1* dxgi_sc = nullptr;
		if (gi_swap_chain_1_)
		{
			dxgi_sc = gi_swap_chain_1_.get();
			THROW_FAILED(dxgi_sc->ResizeBuffers(2, width_, height_, DXGI_FORMAT_R8G8B8A8_UNORM,
				DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
		}
		else
		{
			DXGI_SWAP_CHAIN_DESC1 sc_desc1;
			ZeroMemory(&sc_desc1, sizeof(sc_desc1));
			sc_desc1.Width = width_;
			sc_desc1.Height = height_;
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

		//Frame buffers
		gbuffer_fb_ = std::make_shared<FrameBuffer>();
		gbuffer_fb_->Create(width_, height_, 2);

		linear_depth_fb_ = std::make_shared<FrameBuffer>(DXGI_FORMAT_R32_FLOAT);
		linear_depth_fb_->Create(width_, height_, 1);

		lighting_fb_ = std::make_shared<FrameBuffer>();
		lighting_fb_->Create(width_, height_, 1);

		ID3D11Texture2D* frame_buffer = nullptr;
		THROW_FAILED(dxgi_sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&frame_buffer));

		srgb_fb_ = std::make_shared<FrameBuffer>();
		srgb_fb_->Create(width_, height_, frame_buffer);

		frame_buffer->Release();
		frame_buffer = nullptr;

		//Viewport
		D3D11_VIEWPORT viewport;
		viewport.Width = (float)width_;
		viewport.Height = (float)height_;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		d3d_imm_ctx_->RSSetViewports(1, &viewport);
	}

	void Renderer::Destory()
	{
		rs_.clear();
		cam_.reset();

		if (gi_swap_chain_1_)
		{
			gi_swap_chain_1_->SetFullscreenState(false, nullptr);
		}

		gbuffer_fb_.reset();
		linear_depth_fb_.reset();
		lighting_fb_.reset();
		srgb_fb_.reset();

		quad_.reset();

		d3d_effect_.reset();
		d3d_imm_ctx_.reset();
		d3d_device_.reset();

		gi_swap_chain_1_.reset();
		gi_adapter_.reset();
		gi_factory_1_.reset();
		gi_factory_2_.reset();

		::FreeLibrary(mod_d3dcompiler_);
		::FreeLibrary(mod_d3d11_);
		::FreeLibrary(mod_dxgi_);
	}

	void Renderer::LoadEffect(std::string file_path)
	{
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

		d3d_effect_ = MakeCOMPtr(d3d_effect);
	}

	void Renderer::SetCamera(CameraPtr cam)
	{
		cam_ = cam;
	}

	void Renderer::AddRenderable(RenderablePtr r)
	{
		rs_.push_back(r);
	}

	void Renderer::SetAmbientLight(AmbientLightPtr al)
	{
		ambient_light_ = al;
	}

	void Renderer::AddDirectionLight(DirectionLightPtr dl)
	{
		dir_lights_.push_back(dl);
	}

	void Renderer::AddSpotLight(SpotLightPtr sl)
	{
		spot_lights_.push_back(sl);
	}

	void Renderer::Frame()
	{
		ID3DX11EffectTechnique* tech = d3d_effect_->GetTechniqueByName("DeferredRendering");

		//GBuffer pass
		ID3DX11EffectPass* pass = tech->GetPassByName("GBuffer");

		gbuffer_fb_->Clear();
		gbuffer_fb_->Bind();

		cam_->Bind(d3d_effect_.get());
		for (auto i = rs_.begin(); i != rs_.end(); i++)
		{
			(*i)->Render(d3d_effect_.get(), pass);
		}

		//Linear depth pass
		pass = tech->GetPassByName("LinearDepth");

		linear_depth_fb_->Clear();
		linear_depth_fb_->Bind();

		auto var_g_pp_tex = d3d_effect_->GetVariableByName("g_pp_tex")->AsShaderResource();
		auto var_g_near_q_far = d3d_effect_->GetVariableByName("g_near_q_far")->AsVector();

		var_g_pp_tex->SetResource(gbuffer_fb_->RetriveDSShaderResourceView());

		float q = cam_->far_plane_ / (cam_->far_plane_ - cam_->near_plane_);
		Vector4f near_q_far(cam_->near_plane_ * q, q, cam_->far_plane_, 1 / cam_->far_plane_);
		var_g_near_q_far->SetFloatVector((float*)&near_q_far);

		quad_->Render(d3d_effect_.get(), pass);

		//Lighting-kind passes
		lighting_fb_->Clear();
		lighting_fb_->Bind();

		auto var_g_buffer_tex = d3d_effect_->GetVariableByName("g_buffer_tex")->AsShaderResource();
		auto var_g_buffer_1_tex = d3d_effect_->GetVariableByName("g_buffer_1_tex")->AsShaderResource();
		var_g_buffer_tex->SetResource(gbuffer_fb_->RetriveRTShaderResourceView(0));
		var_g_buffer_1_tex->SetResource(gbuffer_fb_->RetriveRTShaderResourceView(1));

		//Ambient lighting pass
		pass = tech->GetPassByName("AmbientLighting");

		ambient_light_->Bind(d3d_effect_.get(), cam_.get());

		quad_->Render(d3d_effect_.get(), pass);

		//Direction lighting pass for each
		pass = tech->GetPassByName("DirectionLighting");

		for (auto i : dir_lights_)
		{
			i->Bind(d3d_effect_.get(), cam_.get());

			quad_->Render(d3d_effect_.get(), pass);
		}

		//Spot lighting pass for each
		pass = tech->GetPassByName("SpotLighting");

		auto var_g_depth_tex = d3d_effect_->GetVariableByName("g_depth_tex")->AsShaderResource();
		var_g_depth_tex->SetResource(linear_depth_fb_->RetriveRTShaderResourceView(0));

		for (auto i : spot_lights_)
		{
			i->Bind(d3d_effect_.get(), cam_.get());

			quad_->Render(d3d_effect_.get(), pass);
		}

		//SRGBCorrection pass
		srgb_fb_->Clear();
		srgb_fb_->Bind();

		pass = tech->GetPassByName("SRGBCorrection");

		var_g_pp_tex->SetResource(lighting_fb_->RetriveRTShaderResourceView(0));

		quad_->Render(d3d_effect_.get(), pass);

		gi_swap_chain_1_->Present(0, 0);
	}

	IDXGISwapChain1* Renderer::DXGISwapChain()
	{
		return gi_swap_chain_1_.get();
	}

	ID3D11Device* Renderer::D3DDevice()
	{
		return d3d_device_.get();
	}

	ID3D11DeviceContext* Renderer::D3DContext()
	{
		return d3d_imm_ctx_.get();
	}

	HRESULT Renderer::D3DCompile(std::string const & src_data, const char* entry_point, const char* target,
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

	ID3D11Texture2D* Renderer::D3DCreateTexture2D(UINT width, UINT height, int fmt, UINT bind_flags)
	{
		D3D11_TEXTURE2D_DESC d3d_tex_desc;
		ZeroMemory(&d3d_tex_desc, sizeof(d3d_tex_desc));
		d3d_tex_desc.Width = width_;
		d3d_tex_desc.Height = height_;
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

	ID3D11DepthStencilView* Renderer::D3DCreateDepthStencilView(ID3D11Texture2D* tex, int fmt)
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

	ID3D11RenderTargetView* Renderer::D3DCreateRenderTargetView(ID3D11Texture2D* tex)
	{
		ID3D11RenderTargetView* d3d_rtv = nullptr;
		THROW_FAILED(d3d_device_->CreateRenderTargetView(tex, NULL, &d3d_rtv));

		return d3d_rtv;
	}

}