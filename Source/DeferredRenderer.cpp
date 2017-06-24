#include "stdafx.h"
#include "Light.h"
#include "Camera.h"
#include "DeferredRenderer.h"
#include "FrameBuffer.h"
#include "Renderable.h"
#include "ImageBasedProcess.h"
#include "APIContext.h"
#include "FrameStat.h"


namespace zeta
{

	DeferredRenderer::DeferredRenderer()
	{
	}

	DeferredRenderer::~DeferredRenderer()
	{
		rs_.clear();
		cam_.reset();

		if (APIContext::Instance().DXGISwapChain())
		{
			APIContext::Instance().DXGISwapChain()->SetFullscreenState(false, nullptr);
		}

		gbuffer_fb_.reset();
		linear_depth_fb_.reset();
		lighting_fb_.reset();
		shading_fb_.reset();
		color_grading_fb_.reset();
		hdr_fb_.reset();
		fxaa_fb_.reset();

		quad_.reset();
		skybox_.reset();

		color_grading_pp_.reset();
		hdr_pp_.reset();
		fxaa_pp_.reset();
	}

	void DeferredRenderer::Create(HWND wnd, uint32_t width, uint32_t height)
	{
		APIContext::Instance().Create(wnd);

		quad_ = std::make_shared<QuadRenderable>();

		APIContext::Instance().GetEffect("Shader/GBuffer.fx");
		APIContext::Instance().GetEffect("Shader/Lighting.fx");

		hdr_pp_ = std::make_shared<HDRPostProcess>();
		fxaa_pp_ = std::make_shared<FXAAPostProcess>();
		color_grading_pp_ = std::make_shared<ColorGradingPostProcess>();

		this->Resize(width, height);
	}

	void DeferredRenderer::Resize(uint32_t width, uint32_t height)
	{
		if (!APIContext::Instance().Wnd())
		{
			return;
		}

		APIContext::Instance().D3DImmContext()->OMSetRenderTargets(0, 0, 0);
		APIContext::Instance().D3DImmContext()->OMSetDepthStencilState(0, 0);

		gbuffer_fb_.reset();
		linear_depth_fb_.reset();
		lighting_fb_.reset();
		shading_fb_.reset();
		hdr_fb_.reset();
		fxaa_fb_.reset();
		color_grading_fb_.reset();
		color_grading_pp_->SetOutput(nullptr);

		//SwapChain
		APIContext::Instance().ResizeSwapChain(width, height);

		//Frame buffers
		FrameBuffer::VIEW_DESC vd = { width, height, DXGI_FORMAT_R8G8B8A8_UNORM, nullptr };
		std::array<FrameBuffer::VIEW_DESC, 2> vds = { vd, vd };
		gbuffer_fb_ = std::make_shared<FrameBuffer>();
		gbuffer_fb_->Create(vds.data(), 2);

		linear_depth_fb_ = std::make_shared<FrameBuffer>();
		linear_depth_fb_->Create({ width, height, DXGI_FORMAT_R32_FLOAT, nullptr });

		lighting_fb_ = std::make_shared<FrameBuffer>();
		lighting_fb_->Create({ width, height, DXGI_FORMAT_R11G11B10_FLOAT, nullptr });

		shading_fb_ = std::make_shared<FrameBuffer>();
		shading_fb_->Create({ width, height, DXGI_FORMAT_R11G11B10_FLOAT, nullptr });

		hdr_fb_ = std::make_shared<FrameBuffer>();
		hdr_fb_->Create({ width, height, DXGI_FORMAT_R11G11B10_FLOAT, nullptr });

		fxaa_fb_ = std::make_shared<FrameBuffer>();
		fxaa_fb_->Create({ width, height, DXGI_FORMAT_R11G11B10_FLOAT, nullptr });

		ID3D11Texture2D* frame_buffer = nullptr;
		THROW_FAILED(APIContext::Instance().DXGISwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&frame_buffer));

		color_grading_fb_ = std::make_shared<FrameBuffer>();
		color_grading_fb_->Create({ width, height, DXGI_FORMAT_R8G8B8A8_UNORM, frame_buffer });

		frame_buffer->Release();
		frame_buffer = nullptr;

		hdr_pp_->Initialize(width, height);
		fxaa_pp_->Initialize(width, height);
	}

	void DeferredRenderer::SetCamera(CameraPtr cam)
	{
		cam_ = cam;
	}

	CameraPtr DeferredRenderer::GetCamera() const
	{
		return cam_;
	}

	void DeferredRenderer::SetSkyBox(SkyBoxRenderablePtr skybox)
	{
		skybox_ = skybox;
	}

	void DeferredRenderer::AddRenderable(RenderablePtr r)
	{
		rs_.push_back(r);
	}

	void DeferredRenderer::SetAmbientLight(AmbientLightPtr l)
	{
		ambient_light_ = l;
	}

	void DeferredRenderer::SetSkyLight(SkylightPtr l)
	{
		skylight_ = l;
	}

	void DeferredRenderer::AddDirectionLight(DirectionalLightPtr l)
	{
		dir_lights_.push_back(l);
	}

	void DeferredRenderer::AddSpotLight(SpotLightPtr l)
	{
		spot_lights_.push_back(l);
	}

	void DeferredRenderer::Frame()
	{
		//GBuffer pass
		ID3DX11Effect* effect = APIContext::Instance().GetEffect("Shader/GBuffer.fx");
		ID3DX11EffectTechnique* tech = effect->GetTechniqueByName("GBuffer");
		ID3DX11EffectPass* pass = tech->GetPassByName("GBufferGen");

		gbuffer_fb_->Bind();
		gbuffer_fb_->Clear();

		cam_->Bind(effect);
		for (auto i : rs_)
		{
			i->Render(effect, pass);
		}

		//Linear depth pass
		effect = APIContext::Instance().GetEffect("Shader/Lighting.fx");
		tech = effect->GetTechniqueByName("Lighting");
		pass = tech->GetPassByName("LinearDepth");

		cam_->Bind(effect);

		linear_depth_fb_->Bind();
		linear_depth_fb_->Clear();

		float q = cam_->far_plane_ / (cam_->far_plane_ - cam_->near_plane_);
		Vector4f near_q_far(cam_->near_plane_ * q, q, cam_->far_plane_, 1 / cam_->far_plane_);

		SetEffectVar(effect, "g_tex", gbuffer_fb_->RetriveDSShaderResourceView());
		SetEffectVar(effect, "g_near_q_far", near_q_far);

		quad_->Render(effect, pass);

		//Lighting-kind passes
		lighting_fb_->Bind();
		lighting_fb_->Clear();

		SetEffectVar(effect, "g_buffer_tex", gbuffer_fb_->RetriveRTShaderResourceView(0));
		SetEffectVar(effect, "g_buffer_1_tex", gbuffer_fb_->RetriveRTShaderResourceView(1));

		//Ambient lighting pass
		pass = tech->GetPassByName("AmbientLighting");

		ambient_light_->Bind(effect, cam_.get());

		quad_->Render(effect, pass);

		//Skylight lighting pass


		//Direction lighting pass for each
		pass = tech->GetPassByName("DirectionLighting");

		for (auto i : dir_lights_)
		{
			i->Bind(effect, cam_.get());

			quad_->Render(effect, pass);
		}

		//Spot lighting pass for each
		pass = tech->GetPassByName("SpotLighting");

		SetEffectVar(effect, "g_depth_tex", linear_depth_fb_->RetriveRTShaderResourceView(0));
		Matrix inv_proj = Inverse(cam_->proj_);
		SetEffectVar(effect, "g_inv_proj_mat", inv_proj);

		for (auto i : spot_lights_)
		{
			i->Bind(effect, cam_.get());

			quad_->Render(effect, pass);
		}

		//Copy depth pass
		shading_fb_->Bind();
		shading_fb_->Clear();

		pass = tech->GetPassByName("CopyShadingDepth");

		SetEffectVar(effect, "g_shading_tex", lighting_fb_->RetriveRTShaderResourceView(0));
		SetEffectVar(effect, "g_depth_tex", gbuffer_fb_->RetriveDSShaderResourceView());

		quad_->Render(effect, pass);

		//SkyBox pass
		if (skybox_)
		{
			pass = tech->GetPassByName("SkyBoxShading");

			skybox_->SetCamera(cam_);
			skybox_->Render(effect, pass);
		}

		//HDR post process
		hdr_pp_->SetInputDefault(shading_fb_);
		hdr_pp_->SetOutput(hdr_fb_);
		hdr_pp_->Apply();

		//FXAA post process
		fxaa_pp_->SetInputDefault(hdr_fb_);
		fxaa_pp_->SetOutput(fxaa_fb_);
		fxaa_pp_->Apply();

		//SRGBCorrection post process
		color_grading_pp_->SetInputDefault(fxaa_fb_);
		color_grading_pp_->SetOutput(color_grading_fb_);
		color_grading_pp_->Apply();

		//Present
		APIContext::Instance().DXGISwapChain()->Present(0, 0);

		this->UpdateStat();
	}

	void DeferredRenderer::UpdateStat()
	{
		FrameStat::Instance().frame_time_ = timer_.Elapsed();
		timer_.Restart();
		FrameStat::Instance().frame_count_++;
	}

}