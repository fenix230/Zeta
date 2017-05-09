#include "stdafx.h"
#include "ImageBasedProcess.h"
#include "FrameBuffer.h"
#include "Renderable.h"
#include "Renderer.h"


namespace zeta
{
	void ImageBasedProcess::SetInput(FrameBufferPtr fb, std::string pin_name, int srv_index)
	{
		input_fbs_.push_back(fb);
		input_pin_names_.push_back(pin_name);
		input_srv_indexs_.push_back(srv_index);
	}

	void ImageBasedProcess::SetInputDefault(FrameBufferPtr fb)
	{
		this->SetInput(fb, "g_tex", 0);
	}

	void ImageBasedProcess::SetOutput(FrameBufferPtr fb)
	{
		output_fb_ = fb;
	}

	FrameBufferPtr ImageBasedProcess::GetOutput()
	{
		return output_fb_;
	}

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

	ID3DX11EffectPtr OnePassPostProcess::Effect()
	{
		return effect_;
	}

	void OnePassPostProcess::LoadFX(std::string fx_file, std::string tech_name, std::string pass_name)
	{
		ID3DX11EffectPtr effect = MakeCOMPtr(Renderer::Instance().LoadEffect(fx_file));
		ID3DX11EffectTechnique* tech = effect->GetTechniqueByName(tech_name.c_str());
		ID3DX11EffectPass* pass = tech->GetPassByName(pass_name.c_str());
		this->FX(effect, tech, pass);
	}

	void OnePassPostProcess::Apply()
	{
		output_fb_->Clear();
		output_fb_->Bind();

		for (size_t i = 0; i != input_fbs_.size(); i++)
		{
			auto var_g_tex = effect_->GetVariableByName(input_pin_names_[i].c_str())->AsShaderResource();
			var_g_tex->SetResource(input_fbs_[i]->RetriveSRV(input_srv_indexs_[i]));
		}

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
		this->CalcSampleOffsets(output_fb_->Width(), output_fb_->Height());

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
		output_fb_ = adapted_texs_[!last_index_];

		auto var_g_last_lum_tex = effect_->GetVariableByName("g_last_lum_tex")->AsShaderResource();
		var_g_last_lum_tex->SetResource(adapted_texs_[last_index_]->RetriveRTShaderResourceView(0));
		auto var_g_frame_delta = effect_->GetVariableByName("g_frame_delta")->AsScalar();
		var_g_frame_delta->SetFloat((float)Renderer::Instance().FrameTime());

		last_index_ = !last_index_;

		OnePassPostProcess::Apply();
	}

	LensEffectsPostProcess::LensEffectsPostProcess()
	{
		bufw_ = bufh_ = -1;

		bright_pass_downsampler_ = std::make_shared<OnePassPostProcess>();
		bright_pass_downsampler_->LoadFX("Shader/HDR.fx", "HDR", "SqrBright");

		downsamplers_[0] = std::make_shared<OnePassPostProcess>();
		downsamplers_[0]->LoadFX("Shader/HDR.fx", "HDR", "BilinearCopy");
		downsamplers_[1] = std::make_shared<OnePassPostProcess>();
		downsamplers_[1]->LoadFX("Shader/HDR.fx", "HDR", "BilinearCopy");

		blurs_[0] = std::make_shared<BlurPostProcess>("SeparableGaussian", 8, 1.0f);
		blurs_[1] = std::make_shared<BlurPostProcess>("SeparableGaussian", 8, 1.0f);
		blurs_[2] = std::make_shared<BlurPostProcess>("SeparableGaussian", 8, 1.0f);

		glow_merger_ = std::make_shared<OnePassPostProcess>();
		glow_merger_->LoadFX("Shader/HDR.fx", "HDR", "GlowMerger");
	}

	LensEffectsPostProcess::~LensEffectsPostProcess()
	{

	}

	void LensEffectsPostProcess::SetInputDefault(FrameBufferPtr fb)
	{
		uint32_t width = fb->Width();
		uint32_t height = fb->Height();
		DXGI_FORMAT fmt = fb->Format();

		if (width == bufw_ && height == bufh_)
		{
			return;
		}

		downsample_fbs_.clear();
		glow_fbs_.clear();
		downsample_fbs_.resize(3);
		glow_fbs_.resize(3);

		for (size_t i = 0; i < downsample_fbs_.size(); i++)
		{
			downsample_fbs_[i] = std::make_shared<FrameBuffer>(fmt);
			downsample_fbs_[i]->Create(width / (2 << i), height / (2 << i), 1);

			glow_fbs_[i] = std::make_shared<FrameBuffer>(fmt);
			glow_fbs_[i]->Create(width / (2 << i), height / (2 << i), 1);
		}

		bright_pass_downsampler_->SetInputDefault(fb);
		bright_pass_downsampler_->SetOutput(downsample_fbs_[0]);

		for (size_t i = 0; i != downsamplers_.size(); i++)
		{
			downsamplers_[i]->SetInputDefault(downsample_fbs_[i]);
			downsamplers_[i]->SetOutput(downsample_fbs_[i + 1]);
		}

		for (size_t i = 0; i != blurs_.size(); i++)
		{
			blurs_[i]->SetInputDefault(downsample_fbs_[i]);
			blurs_[i]->SetOutput(glow_fbs_[i]);
		}

		glow_merger_->SetInput(glow_fbs_[0], "g_glow_tex_0", 0);
		glow_merger_->SetInput(glow_fbs_[1], "g_glow_tex_1", 0);
		glow_merger_->SetInput(glow_fbs_[2], "g_glow_tex_2", 0);

		FrameBufferPtr len_fb = std::make_shared<FrameBuffer>(fmt);
		len_fb->Create(width / 2, height / 2, 1);
		glow_merger_->SetOutput(len_fb);

		bufw_ = width;
		bufh_ = height;
	}

	FrameBufferPtr LensEffectsPostProcess::GetOutput()
	{
		return glow_merger_->GetOutput();
	}

	void LensEffectsPostProcess::Apply()
	{
		bright_pass_downsampler_->Apply();

		for (size_t i = 0; i != downsamplers_.size(); i++)
		{
			downsamplers_[i]->Apply();
		}

		for (size_t i = 0; i != blurs_.size(); i++)
		{
			blurs_[i]->Apply();
		}

		glow_merger_->Apply();
	}

	SeparableGaussianFilterPostProcess::SeparableGaussianFilterPostProcess(bool x_dir)
	{
		this->LoadFX("Shader/HDR.fx", "HDR", x_dir ? "BlurX" : "BlurY");
		x_dir_ = x_dir;
	}

	SeparableGaussianFilterPostProcess::~SeparableGaussianFilterPostProcess()
	{

	}

	void SeparableGaussianFilterPostProcess::Apply()
	{
		this->CalSampleOffsets(x_dir_ ? input_fbs_[0]->Width() : input_fbs_[0]->Height(), 3.0f);

		auto var_g_tex_size = effect_->GetVariableByName("g_tex_size")->AsVector();
		var_g_tex_size->SetFloatVector((float*)(&tex_size_));

		auto var_g_color_weight = effect_->GetVariableByName("g_color_weight")->AsScalar();
		var_g_color_weight->SetFloatArray(color_weight_.data(), 0, 8);

		auto var_g_tc_offset = effect_->GetVariableByName("g_tc_offset")->AsScalar();
		var_g_tc_offset->SetFloatArray(tc_offset_.data(), 0, 8);

		OnePassPostProcess::Apply();
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
		color_weight_.resize(8, 0);
		tc_offset_.resize(8, 0);

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

			tc_offset_[i] = (tmp_offset[i * 2] + (1 - frac)) * tu;
			color_weight_[i] = multiplier_ * scale;
		}

		tex_size_ = Vector2f(static_cast<float>(tex_size), 1.0f / tex_size);
	}


	ImageBasedProcessPtr CreateFilter(std::string filter_name, int kernel_radius, float multiplier, bool x_dir)
	{
		if (filter_name == "SeparableGaussian") 
		{
			auto filter = std::make_shared<SeparableGaussianFilterPostProcess>(x_dir);
			filter->KernelRadius(kernel_radius);
			filter->Multiplier(multiplier);
			return filter;
		}

		return nullptr;
	}

	BlurPostProcess::BlurPostProcess(std::string filter_name, int kernel_radius, float multiplier)
	{
		x_filter_ = CreateFilter(filter_name, kernel_radius, multiplier, true);
		y_filter_ = CreateFilter(filter_name, kernel_radius, multiplier, false);
	}

	BlurPostProcess::~BlurPostProcess()
	{

	}

	void BlurPostProcess::SetInputDefault(FrameBufferPtr fb)
	{
		if (!fb_ || fb_->Width() != fb->Width() || fb_->Height() != fb->Height())
		{
			fb_ = std::make_shared<FrameBuffer>();
			fb_->Create(fb->Width(), fb->Height(), 1);
		}

		x_filter_->SetInputDefault(fb);
		x_filter_->SetOutput(fb_);

		y_filter_->SetInputDefault(fb_);
	}

	void BlurPostProcess::SetOutput(FrameBufferPtr output_fb)
	{
		y_filter_->SetOutput(output_fb);
	}

	FrameBufferPtr BlurPostProcess::GetOutput()
	{
		return y_filter_->GetOutput();
	}

	void BlurPostProcess::Apply()
	{
		x_filter_->Apply();
		y_filter_->Apply();
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

	void ImageStatPostProcess::CreateBufferChain(uint32_t width, uint32_t height)
	{
		if (!fbs_.empty() && fbs_[0]->Width() == width && fbs_[0]->Height() == height)
		{
			return;
		}

		fbs_.clear();

		fbs_.push_back(std::make_shared<FrameBuffer>(DXGI_FORMAT_R32_FLOAT));
		fbs_.back()->Create(width, height, 1);
		sum_lums_1st_->SetOutput(fbs_.back());

		for (size_t i = 0; i < sum_lums_.size(); ++i)
		{
			sum_lums_[i]->SetInputDefault(fbs_.back());

			fbs_.push_back(std::make_shared<FrameBuffer>(DXGI_FORMAT_R32_FLOAT));
			fbs_.back()->Create(width, height, 1);
			sum_lums_[i]->SetOutput(fbs_.back());
		}

		adapted_lum_->SetInputDefault(fbs_.back());
	}

	void ImageStatPostProcess::SetInputDefault(FrameBufferPtr fb)
	{
		this->CreateBufferChain(fb->Width(), fb->Height());

		sum_lums_1st_->SetInputDefault(fb);
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

	HDRPostProcess::HDRPostProcess()
	{
		image_stat_ = std::make_shared<ImageStatPostProcess>();

		lens_effects_ = std::make_shared<LensEffectsPostProcess>();

		tone_mapping_ = std::make_shared<OnePassPostProcess>();
		tone_mapping_->LoadFX("Shader/HDR.fx", "HDR", "ToneMapping");
	}

	HDRPostProcess::~HDRPostProcess()
	{

	}

	void HDRPostProcess::SetInputDefault(FrameBufferPtr fb)
	{
		image_stat_->SetInputDefault(fb);

		lens_effects_->SetInputDefault(fb);

		tone_mapping_->SetInput(fb, "g_src_tex", 0);
		tone_mapping_->SetInput(lens_effects_->GetOutput(), "g_bloom_tex", 0);
	}

	void HDRPostProcess::SetOutput(FrameBufferPtr fb)
	{
		tone_mapping_->SetOutput(fb);
	}

	FrameBufferPtr HDRPostProcess::GetOutput()
	{
		return tone_mapping_->GetOutput();
	}

	void HDRPostProcess::Apply()
	{
		image_stat_->Apply();

		lens_effects_->Apply();

		tone_mapping_->SetInput(image_stat_->GetOutput(), "g_lum_tex", 0);
		tone_mapping_->Apply();
	}

	FXAAPostProcess::FXAAPostProcess()
	{
		copy_rgbl_ = std::make_shared<OnePassPostProcess>();
		copy_rgbl_->LoadFX("Shader/FXAA.fx", "FXAA", "CopyRGBL");

		fxaa_ = std::make_shared<OnePassPostProcess>();
		fxaa_->LoadFX("Shader/FXAA.fx", "FXAA", "FXAA");
	}

	FXAAPostProcess::~FXAAPostProcess()
	{

	}

	void FXAAPostProcess::SetInputDefault(FrameBufferPtr fb)
	{
		if (!fb_ || fb_->Width() != fb->Width() || fb_->Height() != fb->Height())
		{
			fb_ = std::make_shared<FrameBuffer>(fb->Format());
			fb_->Create(fb->Width(), fb->Height(), 1);
		}

		copy_rgbl_->SetInputDefault(fb);
		copy_rgbl_->SetOutput(fb_);

		fxaa_->SetInputDefault(fb_);
	}

	void FXAAPostProcess::SetOutput(FrameBufferPtr fb)
	{
		fxaa_->SetOutput(fb);
	}

	FrameBufferPtr FXAAPostProcess::GetOutput()
	{
		return fxaa_->GetOutput();
	}

	void FXAAPostProcess::Apply()
	{
		copy_rgbl_->Apply();

		auto var_g_inv_width_height = fxaa_->Effect()->GetVariableByName("g_inv_width_height")->AsVector();
		Vector2f inv_width_height(1.0f / fb_->Width(), 1.0f / fb_->Height());
		var_g_inv_width_height->SetFloatVector((float*)(&inv_width_height));

		fxaa_->Apply();
	}

}