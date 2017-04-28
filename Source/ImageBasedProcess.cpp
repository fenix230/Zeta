#include "stdafx.h"
#include "ImageBasedProcess.h"
#include "FrameBuffer.h"
#include "Renderable.h"
#include "Renderer.h"


namespace zeta
{

	OnePassPostProcess::OnePassPostProcess()
	{
		effect_ = nullptr;
	}

	OnePassPostProcess::~OnePassPostProcess()
	{
		this->Destory();
	}

	void OnePassPostProcess::Destory()
	{
		effect_.reset();
	}

	void OnePassPostProcess::FX(ID3DX11EffectPtr effect, ID3DX11EffectTechnique* tech, ID3DX11EffectPass* pass)
	{
		effect_ = effect;
		tech_ = tech;
		pass_ = pass;
	}

	void OnePassPostProcess::LoadFX(std::string fx_file, std::string tech_name, std::string pass_name)
	{
		ID3DX11EffectPtr effect = MakeCOMPtr(Renderer::Instance().LoadEffect(fx_file));
		ID3DX11EffectTechnique* tech = effect_->GetTechniqueByName(tech_name.c_str());
		ID3DX11EffectPass* pass = tech->GetPassByName(pass_name.c_str());
		this->FX(effect, tech, pass);
	}

	void OnePassPostProcess::Apply(ID3D11ShaderResourceView* input_srv, FrameBufferPtr output_fb)
	{
		output_fb->Clear();
		output_fb->Bind();

		auto var_g_tex = effect_->GetVariableByName("g_tex")->AsShaderResource();
		var_g_tex->SetResource(input_srv);

		Renderer::Instance().Quad()->Render(effect_.get(), pass_);
	}

	SumLumPostProcess::SumLumPostProcess()
	{

	}

	SumLumPostProcess::~SumLumPostProcess()
	{

	}

	void SumLumPostProcess::Apply(ID3D11ShaderResourceView* input_srv, FrameBufferPtr output_fb)
	{
		this->CalcSampleOffsets(output_fb->Width(), output_fb->Height());

		auto var_g_sample_offset1 = effect_->GetVariableByName("g_sample_offset1")->AsVector();
		auto var_g_sample_offset2 = effect_->GetVariableByName("g_sample_offset2")->AsVector();
		var_g_sample_offset1->SetFloatVector((float*)&samples_offset1_);
		var_g_sample_offset2->SetFloatVector((float*)&samples_offset2_);

		OnePassPostProcess::Apply(input_srv, output_fb);
	}

	void SumLumPostProcess::CalcSampleOffsets(uint32_t width, uint32_t height)
	{
		std::vector<Vector4f> tex_coord_offset(2);

		float const tu = 1.0f / width;
		float const tv = 1.0f / height;

		// Sample from the 16 surrounding points.
		int index = 0;
		for (int y = -1; y <= 2; y += 2)
		{
			for (int x = -1; x <= 2; x += 4)
			{
				tex_coord_offset[index].x = (x + 0) * tu;
				tex_coord_offset[index].y = y * tv;
				tex_coord_offset[index].z = (x + 2) * tu;
				tex_coord_offset[index].w = y * tv;

				++index;
			}
		}
		
		samples_offset1_ = tex_coord_offset[0];
		samples_offset2_ = tex_coord_offset[1];
	}

	AdaptedLumPostProcess::AdaptedLumPostProcess()
	{
		adapted_texs_[0] = std::make_shared<FrameBuffer>(DXGI_FORMAT_R32_FLOAT);
		adapted_texs_[1] = std::make_shared<FrameBuffer>(DXGI_FORMAT_R32_FLOAT);
		adapted_texs_[0]->Create(1, 1, 1);
		adapted_texs_[1]->Create(1, 1, 1);
		last_index_ = false;
	}

}