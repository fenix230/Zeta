#pragma once


namespace zeta
{

	class ImageBasedProcess
	{
	public:
		ImageBasedProcess();
		~ImageBasedProcess();

		void Destory();

		void LoadFX(std::string fx_file, std::string tech_name, std::string pass_name);
		void FX(ID3DX11EffectPtr effect, std::string tech_name, std::string pass_name);

		void Apply(ID3D11ShaderResourceView* input_srv, FrameBufferPtr output_fb);

	private:
		std::shared_ptr<ID3DX11Effect> effect_;
		std::string tech_name_;
		std::string pass_name_;
	};

}