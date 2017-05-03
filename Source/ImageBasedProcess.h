#pragma once
#include "Utils.h"


namespace zeta
{

	class ImageBasedProcess
	{
	public:
		virtual void SetInput(ID3D11ShaderResourceView* input_srv) {}
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

		virtual void SetInput(ID3D11ShaderResourceView* input_srv) override;
		virtual void SetOutput(FrameBufferPtr output_fb) override;
		virtual FrameBufferPtr GetOutput() override;
		virtual void Apply() override;

	protected:
		ID3D11ShaderResourceView* input_;
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

		void Create(uint32_t width, uint32_t height);

		virtual void SetInput(ID3D11ShaderResourceView* input_srv) override;
		virtual FrameBufferPtr GetOutput() override;
		virtual void Apply() override;

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

		virtual void SetInput(ID3D11ShaderResourceView* input_srv) override;
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
		SeparableGaussianFilterPostProcess(ID3DX11EffectPtr effect, bool x_dir);
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

}