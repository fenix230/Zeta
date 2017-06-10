#pragma once
#include "Utils.h"


namespace zeta
{

	class APIContext
	{
		APIContext();
		~APIContext();

	public:
		static APIContext& Instance();

		void Create(HWND wnd);

		HWND Wnd();

		void ResizeSwapChain(uint32_t width, uint32_t height);

		ID3DX11Effect* GetEffect(std::string file_path);

		IDXGISwapChain1* DXGISwapChain();

		ID3D11Device* D3DDevice();

		ID3D11DeviceContext* D3DImmContext();

		HRESULT D3DCompile(std::string const & src_data, const char* entry_point, const char* target,
			std::vector<uint8_t>& code, std::string& error_msgs) const;

		ID3D11Texture2D* D3DCreateTexture2D(UINT width, UINT height, DXGI_FORMAT fmt, UINT bind_flags);

		ID3D11DepthStencilView* D3DCreateDepthStencilView(ID3D11Texture2D* tex, DXGI_FORMAT fmt);

		ID3D11RenderTargetView* D3DCreateRenderTargetView(ID3D11Texture2D* tex);

	private:
		HWND wnd_;

		IDXGIFactory1Ptr gi_factory_1_;
		IDXGIFactory2Ptr gi_factory_2_;
		IDXGIAdapterPtr gi_adapter_;
		IDXGISwapChain1Ptr gi_swap_chain_1_;

		ID3D11DevicePtr d3d_device_;
		ID3D11DeviceContextPtr d3d_imm_ctx_;

		std::map<std::string, ID3DX11EffectPtr> effects_;
	};

}