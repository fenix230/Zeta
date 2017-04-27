#include "stdafx.h"
#include "ImageBasedProcess.h"
#include "FrameBuffer.h"
#include "Renderable.h"
#include "Renderer.h"


namespace zeta
{

	ImageBasedProcess::ImageBasedProcess()
	{
		effect_ = nullptr;
	}

	ImageBasedProcess::~ImageBasedProcess()
	{
		this->Destory();
	}

	void ImageBasedProcess::Destory()
	{
		effect_.reset();
	}

	void ImageBasedProcess::FX(ID3DX11EffectPtr effect, std::string tech_name, std::string pass_name)
	{
		effect_ = effect;
		tech_name_ = tech_name;
		pass_name_ = pass_name;
	}

	void ImageBasedProcess::LoadFX(std::string fx_file, std::string tech_name, std::string pass_name)
	{
		effect_ = MakeCOMPtr(Renderer::Instance().LoadEffect(fx_file));
		tech_name_ = tech_name;
		pass_name_ = pass_name;
	}

	void ImageBasedProcess::Apply(ID3D11ShaderResourceView* input_srv, FrameBufferPtr output_fb)
	{
		ID3DX11EffectTechnique* tech = effect_->GetTechniqueByName(tech_name_.c_str());
		ID3DX11EffectPass* pass = tech->GetPassByName(pass_name_.c_str());

		output_fb->Clear();
		output_fb->Bind();

		auto var_g_tex = effect_->GetVariableByName("g_tex")->AsShaderResource();
		var_g_tex->SetResource(input_srv);

		Renderer::Instance().Quad()->Render(effect_.get(), pass);
	}

}