#pragma once
#include "Utils.h"


namespace zeta
{

	class ImageBasedProcess
	{
	public:
		virtual void Apply(ID3D11ShaderResourceView* input_srv, FrameBufferPtr output_fb) = 0;
	};


	class OnePassPostProcess 
		: public ImageBasedProcess
	{
	public:
		OnePassPostProcess();
		virtual ~OnePassPostProcess();

		void LoadFX(std::string fx_file, std::string tech_name, std::string pass_name);

		virtual void Destory();
		virtual void FX(ID3DX11EffectPtr effect, ID3DX11EffectTechnique* tech, ID3DX11EffectPass* pass);

		virtual void Apply(ID3D11ShaderResourceView* input_srv, FrameBufferPtr output_fb) override;

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

		virtual void Apply(ID3D11ShaderResourceView* input_srv, FrameBufferPtr output_fb) override;

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

	private:
		FrameBufferPtr adapted_texs_[2];
		bool last_index_;
	};

}