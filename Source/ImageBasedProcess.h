#pragma once
#include "Utils.h"


namespace zeta
{

	class ImageBasedProcess
	{
	public:
		virtual ~ImageBasedProcess() {}

		virtual void SetInput(FrameBufferPtr input_fb, int srv_index) {}
		virtual void SetOutput(FrameBufferPtr output_fb) {}
		virtual FrameBufferPtr GetOutput() { return nullptr; }
		virtual void Apply() {}
	};


	class OnePassPostProcess 
		: public ImageBasedProcess
	{
	public:
		OnePassPostProcess();
		virtual ~OnePassPostProcess();

		void LoadFX(std::string fx_file, std::string tech_name, std::string pass_name);
		void FX(ID3DX11EffectPtr effect, ID3DX11EffectTechnique* tech, ID3DX11EffectPass* pass);

		virtual void SetInput(FrameBufferPtr input_fb, int srv_index) override;
		virtual void SetOutput(FrameBufferPtr output_fb) override;
		virtual FrameBufferPtr GetOutput() override;
		virtual void Apply() override;

	protected:
		FrameBufferPtr input_;
		int srv_index_;
		FrameBufferPtr output_;

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


	class ImageStatPostProcess
		: public ImageBasedProcess
	{
	public:
		ImageStatPostProcess();
		virtual ~ImageStatPostProcess();

		virtual void SetInput(FrameBufferPtr input_fb, int srv_index) override;
		virtual FrameBufferPtr GetOutput() override;
		virtual void Apply() override;

		void CreateBufferChain(uint32_t width, uint32_t height);

	protected:
		OnePassPostProcessPtr sum_lums_1st_;
		std::vector<OnePassPostProcessPtr> sum_lums_;
		AdaptedLumPostProcessPtr adapted_lum_;
		std::vector<FrameBufferPtr> fbs_;
	};


	class LensEffectsPostProcess
		: public ImageBasedProcess
	{
	public:
		LensEffectsPostProcess();
		virtual ~LensEffectsPostProcess();

		virtual void SetInput(FrameBufferPtr input_fb, int srv_index) override;
		virtual void SetOutput(FrameBufferPtr output_fb) override;
		virtual FrameBufferPtr GetOutput() override;
		virtual void Apply() override;

	private:
		OnePassPostProcessPtr bright_pass_downsampler_;
		std::array<OnePassPostProcessPtr, 2> downsamplers_;
		std::array<OnePassPostProcessPtr, 3> blurs_;
		OnePassPostProcessPtr glow_merger_;
	};


	class SeparableGaussianFilterPostProcess 
		: public OnePassPostProcess
	{
	public:
		SeparableGaussianFilterPostProcess(bool x_dir);
		virtual ~SeparableGaussianFilterPostProcess();

		void KernelRadius(int radius);
		void Multiplier(float multiplier);

	protected:
		float GaussianDistribution(float x, float y, float rho);
		void CalSampleOffsets(uint32_t tex_size, float deviation);

	protected:
		int kernel_radius_;
		float multiplier_;
		bool x_dir_;
	};


	ImageBasedProcessPtr CreateFilter(std::string filter_name, int kernel_radius, float multiplier, bool x_dir);


	class BlurPostProcess
		: public ImageBasedProcess
	{
	public:
		BlurPostProcess(std::string filter_name, int kernel_radius, float multiplier);
		virtual ~BlurPostProcess();

		virtual void SetInput(FrameBufferPtr input_fb, int srv_index) override;
		virtual void SetOutput(FrameBufferPtr output_fb) override;
		virtual FrameBufferPtr GetOutput() override;
		virtual void Apply() override;

	private:
		ImageBasedProcessPtr x_filter_;
		ImageBasedProcessPtr y_filter_;
		FrameBufferPtr fb_;
	};

}