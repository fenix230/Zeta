#pragma once
#include "Utils.h"


namespace zeta
{

	class AmbientLight
	{
	public:
		void Bind(ID3DX11Effect* d3d_effect, Camera* cam);

		Vector3f color_;
	};


	class Skylight
	{
	public:
		Skylight();

		void CreateCubeMap(std::string file_path);
		void CreateCompressedCubeMap(std::string file_path_y, std::string file_path_c);

		void Bind(ID3DX11Effect* d3d_effect, Camera* cam);

	private:
		ID3D11ResourcePtr d3d_tex_;
		ID3D11ShaderResourceViewPtr d3d_srv_;

		ID3D11ResourcePtr d3d_tex_y_;
		ID3D11ShaderResourceViewPtr d3d_srv_y_;

		ID3D11ResourcePtr d3d_tex_c_;
		ID3D11ShaderResourceViewPtr d3d_srv_c_;

		bool compressed_;

		CameraPtr cam_;
	};


	class DirectionalLight
	{
	public:
		void Bind(ID3DX11Effect* d3d_effect, Camera* cam);

		Vector3f dir_;
		Vector3f color_;
	};


	class SpotLight
	{
	public:
		void Bind(ID3DX11Effect* d3d_effect, Camera* cam);

		Vector3f pos_;
		Vector3f dir_;
		Vector3f color_;
		Vector3f falloff_;
		float range_;
		float inner_ang_;
		float outter_ang_;
	};

}