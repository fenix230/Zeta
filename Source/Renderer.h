#pragma once
#include "Utils.h"


namespace zeta
{

	class Renderer
	{
		Renderer();
		~Renderer();

	public:
		static Renderer& Instance();

		void Create(HWND wnd, int width, int height);

		void Resize(int width, int height);

		void SetCamera(CameraPtr cam);
		CameraPtr GetCamera() const;

		void SetSkyBox(SkyBoxRenderablePtr skybox);

		void AddRenderable(RenderablePtr r);

		void SetAmbientLight(AmbientLightPtr al);
		void AddDirectionLight(DirectionLightPtr dl);
		void AddSpotLight(SpotLightPtr sl);

		void Frame();

		void UpdateStat();

		double FrameTime();

		QuadRenderablePtr Quad();

		ID3DX11Effect* GetEffect(std::string file_path);

		IDXGISwapChain1* DXGISwapChain();

		ID3D11Device* D3DDevice();

		ID3D11DeviceContext* D3DContext();

		HRESULT D3DCompile(std::string const & src_data, const char* entry_point, const char* target,
			std::vector<uint8_t>& code, std::string& error_msgs) const;

		ID3D11Texture2D* D3DCreateTexture2D(UINT width, UINT height, DXGI_FORMAT fmt, UINT bind_flags);

		ID3D11DepthStencilView* D3DCreateDepthStencilView(ID3D11Texture2D* tex, DXGI_FORMAT fmt);

		ID3D11RenderTargetView* D3DCreateRenderTargetView(ID3D11Texture2D* tex);

	private:
		HWND wnd_;

		HMODULE mod_d3d11_;
		HMODULE mod_dxgi_;
		HMODULE mod_d3dcompiler_;

		IDXGIFactory1Ptr gi_factory_1_;
		IDXGIFactory2Ptr gi_factory_2_;
		IDXGIAdapterPtr gi_adapter_;
		IDXGISwapChain1Ptr gi_swap_chain_1_;

		ID3D11DevicePtr d3d_device_;
		ID3D11DeviceContextPtr d3d_imm_ctx_;

		FrameBufferPtr gbuffer_fb_;
		FrameBufferPtr linear_depth_fb_;
		FrameBufferPtr lighting_fb_;
		FrameBufferPtr shading_fb_;
		FrameBufferPtr hdr_fb_;
		FrameBufferPtr fxaa_fb_;
		FrameBufferPtr srgb_fb_;

		std::map<std::string, ID3DX11EffectPtr> effects_;

		OnePassPostProcessPtr srgb_pp_;
		HDRPostProcessPtr hdr_pp_;
		FXAAPostProcessPtr fxaa_pp_;

		QuadRenderablePtr quad_; 

		CameraPtr cam_;
		SkyBoxRenderablePtr skybox_;
		std::vector<RenderablePtr> rs_;

		AmbientLightPtr ambient_light_;
		std::vector<DirectionLightPtr> dir_lights_;
		std::vector<SpotLightPtr> spot_lights_;

		Timer timer_;
		double frame_time_;
		uint64_t frame_count_;
	};

}