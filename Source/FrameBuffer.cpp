#include "stdafx.h"
#include "FrameBuffer.h"
#include "Renderer.h"


namespace zeta
{

	FrameBuffer::FrameBuffer()
	{
	}

	FrameBuffer::~FrameBuffer()
	{
		rtv_descs_.clear();
		rtvs_.clear();
		d3d_dsv_tex_.reset();
		d3d_dsv_.reset();
	}

	void FrameBuffer::Create(const VIEW_DESC& rtv_descs)
	{
		this->Create(&rtv_descs, 1);
	}

	void FrameBuffer::Create(const VIEW_DESC* rtv_descs, size_t rtv_count)
	{
		rtv_descs_.clear();
		std::copy_n(rtv_descs, rtv_count, std::back_inserter(rtv_descs_));

		//Render target view
		rtvs_.resize(rtv_count);

		for (size_t i = 0; i != rtv_descs_.size(); i++)
		{
			const VIEW_DESC& desc = rtv_descs_[i];
			RTV& rtv = rtvs_[i];
			if (desc.tex)
			{
				rtv.d3d_rtv_ = MakeCOMPtr(Renderer::Instance().D3DCreateRenderTargetView(desc.tex));
			}
			else
			{
				rtv.d3d_rtv_tex_ = MakeCOMPtr(Renderer::Instance().D3DCreateTexture2D(desc.width, desc.height, desc.format,
					D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE));

				D3D11_RENDER_TARGET_VIEW_DESC d3d_rtv_desc;
				d3d_rtv_desc.Format = desc.format;
				d3d_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				d3d_rtv_desc.Texture2D.MipSlice = 0;

				ID3D11RenderTargetView* d3d_rtv = nullptr;
				THROW_FAILED(Renderer::Instance().D3DDevice()->CreateRenderTargetView(rtv.d3d_rtv_tex_.get(), &d3d_rtv_desc, &d3d_rtv));

				rtv.d3d_rtv_ = MakeCOMPtr(d3d_rtv);
			}
		}

		//Depth stencil view
		uint32_t width = 0;
		uint32_t height = 0;
		for (size_t i = 0; i != rtv_count; i++)
		{
			width = (std::max)(width, this->Width(i));
			height = (std::max)(height, this->Height(i));
		}

		d3d_dsv_tex_ = MakeCOMPtr(Renderer::Instance().D3DCreateTexture2D(width, height,
			DXGI_FORMAT_R24G8_TYPELESS, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE));

		d3d_dsv_ = MakeCOMPtr(Renderer::Instance().D3DCreateDepthStencilView(d3d_dsv_tex_.get(),
			DXGI_FORMAT_D24_UNORM_S8_UINT));
	}

	void FrameBuffer::Clear(Vector4f* c)
	{
		float clean_color[4] = { 0, 0, 0, 0 };
		float* pc = (c != nullptr ? (float*)c : clean_color);

		for (size_t i = 0; i != rtvs_.size(); i++)
		{
			Renderer::Instance().D3DContext()->ClearRenderTargetView(rtvs_[i].d3d_rtv_.get(), pc);
		}

		Renderer::Instance().D3DContext()->ClearDepthStencilView(d3d_dsv_.get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	void FrameBuffer::Bind()
	{
		//Bind render target
		std::vector<ID3D11RenderTargetView*> d3d_rtvs;
		for (size_t i = 0; i != rtvs_.size(); i++)
		{
			d3d_rtvs.push_back(rtvs_[i].d3d_rtv_.get());
		}

		Renderer::Instance().D3DContext()->OMSetRenderTargets((UINT)d3d_rtvs.size(), d3d_rtvs.data(), d3d_dsv_.get());

		//Bind Viewport
		D3D11_VIEWPORT viewport;
		viewport.Width = (float)this->Width(0);
		viewport.Height = (float)this->Height(0);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		Renderer::Instance().D3DContext()->RSSetViewports(1, &viewport);
	}

	DXGI_FORMAT FrameBuffer::Format(size_t index)
	{
		return rtv_descs_[index].format;
	}

	uint32_t FrameBuffer::Width(size_t index)
	{
		return rtv_descs_[index].width;
	}

	uint32_t FrameBuffer::Height(size_t index)
	{
		return rtv_descs_[index].height;
	}

	ID3D11ShaderResourceView* FrameBuffer::RetriveRTShaderResourceView(size_t index)
	{
		if (!rtvs_[index].d3d_srv_)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC d3d_srv_desc;
			d3d_srv_desc.Format = rtv_descs_[index].format;
			d3d_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			d3d_srv_desc.Texture2D.MostDetailedMip = 0;
			d3d_srv_desc.Texture2D.MipLevels = 1;

			ID3D11ShaderResourceView* d3d_srv = nullptr;
			THROW_FAILED(Renderer::Instance().D3DDevice()->CreateShaderResourceView(rtvs_[index].d3d_rtv_tex_.get(), &d3d_srv_desc, &d3d_srv));
			rtvs_[index].d3d_srv_ = MakeCOMPtr(d3d_srv);
		}

		return rtvs_[index].d3d_srv_.get();
	}

	ID3D11ShaderResourceView* FrameBuffer::RetriveDSShaderResourceView()
	{
		if (!d3d_ds_srv_)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC d3d_srv_desc;
			d3d_srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			d3d_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			d3d_srv_desc.Texture2D.MostDetailedMip = 0;
			d3d_srv_desc.Texture2D.MipLevels = 1;

			ID3D11ShaderResourceView* d3d_srv = nullptr;
			THROW_FAILED(Renderer::Instance().D3DDevice()->CreateShaderResourceView(d3d_dsv_tex_.get(), &d3d_srv_desc, &d3d_srv));
			d3d_ds_srv_ = MakeCOMPtr(d3d_srv);
		}
		
		return d3d_ds_srv_.get();
	}

	ID3D11ShaderResourceView* FrameBuffer::RetriveSRV(int index)
	{
		if (index >= 0)
		{
			return this->RetriveRTShaderResourceView((size_t)index);
		}

		return this->RetriveDSShaderResourceView();
	}

}