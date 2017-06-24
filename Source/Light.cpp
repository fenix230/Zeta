#include "stdafx.h"
#include "Light.h"
#include "Camera.h"
#include "APIContext.h"
#include "DDSTextureLoader.h"


namespace zeta
{

	void AmbientLight::Bind(ID3DX11Effect* d3d_effect, Camera* cam)
	{
		Vector3f light_dir(0, 1, 0);
		light_dir = TransformNormal(light_dir, cam->view_);

		SetEffectVar(d3d_effect, "g_light_dir_es", light_dir);
		SetEffectVar(d3d_effect, "g_light_color", color_);
	}


	Skylight::Skylight()
	{
		compressed_ = false;
	}

	void Skylight::CreateCubeMap(std::string file_path)
	{
		compressed_ = false;

		if (!file_path.empty())
		{
			std::wstring wfile_path = ToW(file_path);

			ID3D11Resource* d3d_tex_res = nullptr;
			ID3D11ShaderResourceView* d3d_tex_srv = nullptr;
			if (SUCCEEDED(
				CreateDDSTextureFromFileEx(APIContext::Instance().D3DDevice(), nullptr, wfile_path.c_str(), 0,
					D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, true,
					&d3d_tex_res, &d3d_tex_srv))
				)
			{
				d3d_tex_ = MakeCOMPtr(d3d_tex_res);
				d3d_srv_ = MakeCOMPtr(d3d_tex_srv);
			}
		}
	}

	void Skylight::CreateCompressedCubeMap(std::string file_path_y, std::string file_path_c)
	{
		compressed_ = true;

		if (!file_path_y.empty())
		{
			std::wstring wfile_path = ToW(file_path_y);

			ID3D11Resource* d3d_tex_res = nullptr;
			ID3D11ShaderResourceView* d3d_tex_srv = nullptr;
			if (SUCCEEDED(
				CreateDDSTextureFromFileEx(APIContext::Instance().D3DDevice(), nullptr, wfile_path.c_str(), 0,
					D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, true,
					&d3d_tex_res, &d3d_tex_srv))
				)
			{
				d3d_tex_y_ = MakeCOMPtr(d3d_tex_res);
				d3d_srv_y_ = MakeCOMPtr(d3d_tex_srv);
			}
		}

		if (!file_path_c.empty())
		{
			std::wstring wfile_path = ToW(file_path_c);

			ID3D11Resource* d3d_tex_res = nullptr;
			ID3D11ShaderResourceView* d3d_tex_srv = nullptr;
			if (SUCCEEDED(
				CreateDDSTextureFromFileEx(APIContext::Instance().D3DDevice(), nullptr, wfile_path.c_str(), 0,
					D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, true,
					&d3d_tex_res, &d3d_tex_srv))
				)
			{
				d3d_tex_c_ = MakeCOMPtr(d3d_tex_res);
				d3d_srv_c_ = MakeCOMPtr(d3d_tex_srv);
			}
		}
	}

	void Skylight::Bind(ID3DX11Effect* d3d_effect, Camera* cam)
	{
		Matrix rot_view = cam->view_;
		rot_view.At(3, 0) = 0;
		rot_view.At(3, 1) = 0;
		rot_view.At(3, 2) = 0;
		Matrix proj = cam->proj_;
		Matrix inv_mvp = Inverse(rot_view * proj);

		SetEffectVar(d3d_effect, "g_inv_mvp_mat", inv_mvp);
		SetEffectVar(d3d_effect, "g_cube_tex_compressed", compressed_);

		if (compressed_)
		{
			SetEffectVar(d3d_effect, "g_cube_tex_y", d3d_srv_y_.get());
			SetEffectVar(d3d_effect, "g_cube_tex_c", d3d_srv_c_.get());
		}
		else
		{
			SetEffectVar(d3d_effect, "g_cube_tex", d3d_srv_.get());
		}
	}


	void DirectionalLight::Bind(ID3DX11Effect* d3d_effect, Camera* cam)
	{
		Vector3f light_dir = TransformNormal(dir_, cam->view_);

		SetEffectVar(d3d_effect, "g_light_dir_es", light_dir);
		SetEffectVar(d3d_effect, "g_light_color", color_);
	}


	void SpotLight::Bind(ID3DX11Effect* d3d_effect, Camera* cam)
	{
		Vector3f light_pos = TransformCoord(pos_, cam->view_);
		Vector3f light_dir = TransformNormal(dir_, cam->view_);
		Vector2f cos_cone;
		cos_cone.x = cos(inner_ang_);
		cos_cone.y = cos(outter_ang_);
		Vector4f falloff_range;
		falloff_range.x = falloff_.x;
		falloff_range.y = falloff_.y;
		falloff_range.z = falloff_.z;
		falloff_range.w = range_;

		SetEffectVar(d3d_effect, "g_light_pos_es", light_pos);
		SetEffectVar(d3d_effect, "g_light_dir_es", light_dir);
		SetEffectVar(d3d_effect, "g_light_color", color_);
		SetEffectVar(d3d_effect, "g_light_falloff_range", falloff_range);
		SetEffectVar(d3d_effect, "g_spot_light_cos_cone", cos_cone);
	}

}