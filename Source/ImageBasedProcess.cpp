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
		ID3DX11EffectTechnique* tech = effect->GetTechniqueByName(tech_name.c_str());
		ID3DX11EffectPass* pass = tech->GetPassByName(pass_name.c_str());
		this->FX(effect, tech, pass);
	}

	void OnePassPostProcess::SetInput(ID3D11ShaderResourceView* input_srv)
	{
		input_ = input_srv;
	}

	void OnePassPostProcess::SetOutput(FrameBufferPtr output_fb)
	{
		output_ = output_fb;
	}

	FrameBufferPtr OnePassPostProcess::GetOutput()
	{
		return output_;
	}

	void OnePassPostProcess::Apply()
	{
		output_->Clear();
		output_->Bind();

		auto var_g_tex = effect_->GetVariableByName("g_tex")->AsShaderResource();
		var_g_tex->SetResource(input_);

		Renderer::Instance().Quad()->Render(effect_.get(), pass_);
	}

	SumLumPostProcess::SumLumPostProcess()
	{

	}

	SumLumPostProcess::~SumLumPostProcess()
	{

	}

	void SumLumPostProcess::Apply()
	{
		this->CalcSampleOffsets(output_->Width(), output_->Height());

		auto var_g_sample_offset1 = effect_->GetVariableByName("g_sample_offset1")->AsVector();
		auto var_g_sample_offset2 = effect_->GetVariableByName("g_sample_offset2")->AsVector();
		var_g_sample_offset1->SetFloatVector((float*)&samples_offset1_);
		var_g_sample_offset2->SetFloatVector((float*)&samples_offset2_);

		OnePassPostProcess::Apply();
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

		this->LoadFX("Shader/HDR.fx", "HDR", "AdaptedLum");
	}

	AdaptedLumPostProcess::~AdaptedLumPostProcess()
	{
		adapted_texs_[0].reset();
		adapted_texs_[1].reset();
	}

	void AdaptedLumPostProcess::Apply()
	{
		output_ = adapted_texs_[!last_index_];

		auto var_g_last_lum_tex = effect_->GetVariableByName("g_last_lum_tex")->AsShaderResource();
		var_g_last_lum_tex->SetResource(adapted_texs_[last_index_]->RetriveRTShaderResourceView(0));
		auto var_g_frame_delta = effect_->GetVariableByName("g_frame_delta")->AsScalar();
		var_g_frame_delta->SetFloat((float)Renderer::Instance().FrameTime());

		last_index_ = !last_index_;

		OnePassPostProcess::Apply();
	}

	ImageStatPostProcess::ImageStatPostProcess()
	{
		sum_lums_1st_ = std::make_shared<SumLumPostProcess>();
		sum_lums_1st_->LoadFX("Shader/HDR.fx", "HDR", "SumLumLog");

		sum_lums_.resize(3);
		for (size_t i = 0; i < sum_lums_.size(); ++i)
		{
			sum_lums_[i] = std::make_shared<SumLumPostProcess>();
			sum_lums_[i]->LoadFX("Shader/HDR.fx", "HDR", "SumLumIterative");
		}

		adapted_lum_ = std::make_shared<AdaptedLumPostProcess>();
	}

	ImageStatPostProcess::~ImageStatPostProcess()
	{
		sum_lums_1st_.reset();
		sum_lums_.clear();
		adapted_lum_.reset();
	}

	void ImageStatPostProcess::Create(uint32_t width, uint32_t height)
	{
		fbs_.clear();

		fbs_.push_back(std::make_shared<FrameBuffer>(DXGI_FORMAT_R32_FLOAT));
		fbs_.back()->Create(width, height, 1);
		sum_lums_1st_->SetOutput(fbs_.back());

		for (size_t i = 0; i < sum_lums_.size(); ++i)
		{
			sum_lums_[i]->SetInput(fbs_.back()->RetriveRTShaderResourceView(0));

			fbs_.push_back(std::make_shared<FrameBuffer>(DXGI_FORMAT_R32_FLOAT));
			fbs_.back()->Create(width, height, 1);
			sum_lums_[i]->SetOutput(fbs_.back());
		}
		
		adapted_lum_->SetInput(fbs_.back()->RetriveRTShaderResourceView(0));
	}

	void ImageStatPostProcess::SetInput(ID3D11ShaderResourceView* input_srv)
	{
		sum_lums_1st_->SetInput(input_srv);
	}

	FrameBufferPtr ImageStatPostProcess::GetOutput()
	{
		return adapted_lum_->GetOutput();
	}

	void ImageStatPostProcess::Apply()
	{
		// 降采样4x4 log
		sum_lums_1st_->Apply();

		for (size_t i = 0; i < sum_lums_.size(); ++i)
		{
			// 降采样4x4
			sum_lums_[i]->Apply();
		}

		adapted_lum_->Apply();
	}

	LensEffectsPostProcess::LensEffectsPostProcess()
	{

	}

	LensEffectsPostProcess::~LensEffectsPostProcess()
	{

	}

	void LensEffectsPostProcess::SetInput(ID3D11ShaderResourceView* input_srv)
	{

	}

	void LensEffectsPostProcess::SetOutput(FrameBufferPtr output_fb)
	{

	}

	FrameBufferPtr LensEffectsPostProcess::GetOutput()
	{
		return nullptr;
	}

	void LensEffectsPostProcess::Apply()
	{

	}

	SeparableGaussianFilterPostProcess::SeparableGaussianFilterPostProcess(ID3DX11EffectPtr effect, bool x_dir)
	{
		ID3DX11EffectTechnique* tech = effect->GetTechniqueByName("HDR");
		ID3DX11EffectPass* pass = tech->GetPassByName(x_dir ? "BlurX" : "BlurY");

		this->FX(effect, tech, pass);
		x_dir_ = x_dir;
	}

	SeparableGaussianFilterPostProcess::~SeparableGaussianFilterPostProcess()
	{

	}

	void SeparableGaussianFilterPostProcess::KernelRadius(int radius)
	{
		kernel_radius_ = radius;
	}

	void SeparableGaussianFilterPostProcess::Multiplier(float multiplier)
	{
		multiplier_ = multiplier;
	}

	float SeparableGaussianFilterPostProcess::GaussianDistribution(float x, float y, float rho)
	{
		float g = 1.0f / sqrt(2.0f * XM_PI * rho * rho);
		g *= exp(-(x * x + y * y) / (2 * rho * rho));
		return g;
	}

	void SeparableGaussianFilterPostProcess::CalSampleOffsets(uint32_t tex_size, float deviation)
	{
		std::vector<float> color_weight(8, 0);
		std::vector<float> tex_coord_offset(8, 0);

		std::vector<float> tmp_weights(kernel_radius_ * 2, 0);
		std::vector<float> tmp_offset(kernel_radius_ * 2, 0);

		float const tu = 1.0f / tex_size;

		float sum_weight = 0;
		for (int i = 0; i < 2 * kernel_radius_; ++i)
		{
			float weight = this->GaussianDistribution(static_cast<float>(i - kernel_radius_), 0, kernel_radius_ / deviation);
			tmp_weights[i] = weight;
			sum_weight += weight;
		}
		for (int i = 0; i < 2 * kernel_radius_; ++i)
		{
			tmp_weights[i] /= sum_weight;
		}

		// Fill the offsets
		for (int i = 0; i < kernel_radius_; ++i)
		{
			tmp_offset[i] = static_cast<float>(i - kernel_radius_);
			tmp_offset[i + kernel_radius_] = static_cast<float>(i);
		}

		// Bilinear filtering taps
		// Ordering is left to right.
		for (int i = 0; i < kernel_radius_; ++i)
		{
			float const scale = tmp_weights[i * 2] + tmp_weights[i * 2 + 1];
			float const frac = tmp_weights[i * 2] / scale;

			tex_coord_offset[i] = (tmp_offset[i * 2] + (1 - frac)) * tu;
			color_weight[i] = multiplier_ * scale;
		}

		auto var_g_tex_size = effect_->GetVariableByName("g_tex_size")->AsVector();
		Vector2f ts(static_cast<float>(tex_size), 1.0f / tex_size);
		var_g_tex_size->SetFloatVector((float*)(&ts));

		auto var_g_color_weight = effect_->GetVariableByName("g_color_weight")->AsScalar();
		var_g_color_weight->SetFloatArray(color_weight.data(), 0, 8);

		auto var_g_tc_offset = effect_->GetVariableByName("g_tc_offset")->AsScalar();
		var_g_tc_offset->SetFloatArray(tex_coord_offset.data(), 0, 8);
	}

}