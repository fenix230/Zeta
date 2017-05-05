#pragma once
#include "Utils.h"


namespace zeta
{

	class FrameBuffer
	{
	public:
		FrameBuffer();
		FrameBuffer(DXGI_FORMAT rtv_fmt);
		virtual ~FrameBuffer();

		void Create(uint32_t width, uint32_t height, ID3D11Texture2D* sc_buffer);

		void Create(uint32_t width, uint32_t height, size_t rtv_count);

		void Create(uint32_t width, uint32_t height, size_t rtv_count, ID3D11Texture2D* sc_buffer, size_t sc_index);

		void Clear(Vector4f* c = nullptr);

		void Bind();

		DXGI_FORMAT Format();

		uint32_t Width();
		uint32_t Height();

		ID3D11ShaderResourceView* RetriveRTShaderResourceView(size_t index);

		ID3D11ShaderResourceView* RetriveDSShaderResourceView();

		ID3D11ShaderResourceView* RetriveSRV(int index);

	private:
		DXGI_FORMAT rtv_fmt_;

		uint32_t width_;
		uint32_t height_;

		struct RTV
		{
			ID3D11Texture2DPtr d3d_rtv_tex_;
			ID3D11RenderTargetViewPtr d3d_rtv_;
			ID3D11ShaderResourceViewPtr d3d_srv_;
		};
		std::vector<RTV> rtvs_;

		ID3D11Texture2DPtr d3d_dsv_tex_;
		ID3D11DepthStencilViewPtr d3d_dsv_;
		ID3D11ShaderResourceViewPtr d3d_ds_srv_;
	};

}