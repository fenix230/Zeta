#pragma once
#include "Utils.h"


namespace zeta
{

	class FrameBuffer
	{
	public:
		FrameBuffer();
		virtual ~FrameBuffer();

		struct VIEW_DESC
		{
			uint32_t width;
			uint32_t height;
			DXGI_FORMAT format;
			ID3D11Texture2D* tex;
		};
		void Create(const VIEW_DESC& rtv_descs);
		void Create(const VIEW_DESC* rtv_descs, size_t rtv_count);

		void Clear(Vector4f* c = nullptr);

		void Bind();

		uint32_t Width(size_t index);
		uint32_t Height(size_t index);
		DXGI_FORMAT Format(size_t index);

		ID3D11ShaderResourceView* RetriveRTShaderResourceView(size_t index);

		ID3D11ShaderResourceView* RetriveDSShaderResourceView();

		ID3D11ShaderResourceView* RetriveSRV(int index);

	private:
		std::vector<VIEW_DESC> rtv_descs_;

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