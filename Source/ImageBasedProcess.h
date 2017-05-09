#pragma once
#include "Utils.h"


namespace zeta
{

	class ImageBasedProcess
	{
	public:
		virtual ~ImageBasedProcess() {}

		virtual void SetInput(FrameBufferPtr fb, std::string pin_name, int srv_index);
		virtual void SetInputDefault(FrameBufferPtr fb);
		virtual void SetOutput(FrameBufferPtr fb);
		virtual FrameBufferPtr GetOutput();

		virtual void Apply() = 0;

	protected:
		std::vector<FrameBufferPtr> input_fbs_;
		std::vector<std::string> input_pin_names_;
		std::vector<int> input_srv_indexs_;
		FrameBufferPtr output_fb_;
	};


	class OnePassPostProcess 
		: public ImageBasedProcess
	{
	public:
		OnePassPostProcess();
		virtual ~OnePassPostProcess();

		void LoadFX(std::string fx_file, std::string tech_name, std::string pass_name);
		void FX(ID3DX11EffectPtr effect, ID3DX11EffectTechnique* tech, ID3DX11EffectPass* pass);
		ID3DX11EffectPtr Effect();

		virtual void Apply() override;

	protected:
		ID3DX11EffectPtr effect_;
		ID3DX11EffectTechnique* tech_;
		ID3DX11EffectPass* pass_;
	};


	class SumLumPostProcess 
		: public OnePassPostProcess
	{
	public:
		SumLumPostProcess();
		virtual ~SumLumPostProcess();

		virtual void Apply() override;

	private:
		void CalcSampleOffsets(uint32_t width, uint32_t height);

	protected:
		Vector4f samples_offset1_;
		Vector4f samples_offset2_;
	};


	class AdaptedLumPostProcess 
		: public OnePassPostProcess
	{
	public:
		AdaptedLumPostProcess();
		virtual ~AdaptedLumPostProcess();

		virtual void SetOutput(FrameBufferPtr output_fb) override {}
		virtual void Apply() override;

	private:
		FrameBufferPtr adapted_texs_[2];
		bool last_index_;
	};


	class LensEffectsPostProcess
		: public ImageBasedProcess
	{
	public:
		LensEffectsPostProcess();
		virtual ~LensEffectsPostProcess();

		virtual void SetInput(FrameBufferPtr fb, std::string pin_name, int srv_index) override {}
		virtual void SetInputDefault(FrameBufferPtr fb) override;
		virtual void SetOutput(FrameBufferPtr output_fb) override {}
		virtual FrameBufferPtr GetOutput() override;
		virtual void Apply() override;

	private:
		int bufw_, bufh_;
		std::vector<FrameBufferPtr> downsample_fbs_;
		std::vector<FrameBufferPtr> glow_fbs_;

		OnePassPostProcessPtr bright_pass_downsampler_;
		std::array<OnePassPostProcessPtr, 2> downsamplers_;
		std::array<ImageBasedProcessPtr, 3> blurs_;
		OnePassPostProcessPtr glow_merger_;
	};


	class SeparableGaussianFilterPostProcess 
		: public OnePassPostProcess
	{
	public:
		SeparableGaussianFilterPostProcess(bool x_dir);
		virtual ~SeparableGaussianFilterPostProcess();

		virtual void Apply() override;

		void KernelRadius(int radius);
		void Multiplier(float multiplier);

	protected:
		float GaussianDistribution(float x, float y, float rho);
		void CalSampleOffsets(uint32_t tex_size, float deviation);

	protected:
		int kernel_radius_;
		float multiplier_;
		bool x_dir_;
		Vector2f tex_size_;
		std::vector<float> color_weight_;
		std::vector<float> tc_offset_;
	};


	ImageBasedProcessPtr CreateFilter(std::string filter_name, int kernel_radius, float multiplier, bool x_dir);


	class BlurPostProcess
		: public ImageBasedProcess
	{
	public:
		BlurPostProcess(std::string filter_name, int kernel_radius, float multiplier);
		virtual ~BlurPostProcess();

		virtual void SetInput(FrameBufferPtr fb, std::string pin_name, int srv_index) override {}
		virtual void SetInputDefault(FrameBufferPtr fb) override;
		virtual void SetOutput(FrameBufferPtr output_fb) override;
		virtual FrameBufferPtr GetOutput() override;
		virtual void Apply() override;

	private:
		ImageBasedProcessPtr x_filter_;
		ImageBasedProcessPtr y_filter_;
		FrameBufferPtr fb_;
	};


	class ImageStatPostProcess
		: public ImageBasedProcess
	{
	public:
		ImageStatPostProcess();
		virtual ~ImageStatPostProcess();

		virtual void SetInput(FrameBufferPtr fb, std::string pin_name, int srv_index) override {}
		virtual void SetInputDefault(FrameBufferPtr fb) override;
		virtual void SetOutput(FrameBufferPtr fb) override {}
		virtual FrameBufferPtr GetOutput() override;
		virtual void Apply() override;

		void CreateBufferChain(uint32_t width, uint32_t height);

	protected:
		OnePassPostProcessPtr sum_lums_1st_;
		std::vector<OnePassPostProcessPtr> sum_lums_;
		AdaptedLumPostProcessPtr adapted_lum_;
		std::vector<FrameBufferPtr> fbs_;
	};


	class HDRPostProcess
		: public ImageBasedProcess
	{
	public:
		HDRPostProcess();
		virtual ~HDRPostProcess();

		virtual void SetInput(FrameBufferPtr fb, std::string pin_name, int srv_index) override {}
		virtual void SetInputDefault(FrameBufferPtr fb) override;
		virtual void SetOutput(FrameBufferPtr fb) override;
		virtual FrameBufferPtr GetOutput() override;
		virtual void Apply() override;

	private:
		ImageStatPostProcessPtr image_stat_;
		LensEffectsPostProcessPtr lens_effects_;
		OnePassPostProcessPtr tone_mapping_;
	};


	class FXAAPostProcess
		: public ImageBasedProcess
	{
	public:
		FXAAPostProcess();
		virtual ~FXAAPostProcess();

		virtual void SetInput(FrameBufferPtr fb, std::string pin_name, int srv_index) override {}
		virtual void SetInputDefault(FrameBufferPtr fb) override;
		virtual void SetOutput(FrameBufferPtr fb) override;
		virtual FrameBufferPtr GetOutput() override;
		virtual void Apply() override;

	private:
		OnePassPostProcessPtr copy_rgbl_;
		OnePassPostProcessPtr fxaa_;
		FrameBufferPtr fb_;
	};

}