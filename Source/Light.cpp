#include "stdafx.h"
#include "Light.h"
#include "Camera.h"


namespace zeta
{

	void AmbientLight::Bind(ID3DX11Effect* d3d_effect, Camera* cam)
	{
		auto var_g_light_dir_es = d3d_effect->GetVariableByName("g_light_dir_es")->AsVector();
		auto var_g_light_color = d3d_effect->GetVariableByName("g_light_color")->AsVector();

		Vector3f light_dir(0, 1, 0);
		light_dir = TransformNormal(light_dir, cam->view_);

		var_g_light_dir_es->SetFloatVector((float*)&light_dir);
		var_g_light_color->SetFloatVector((float*)&color_);
	}


	void DirectionLight::Bind(ID3DX11Effect* d3d_effect, Camera* cam)
	{
		auto var_g_light_dir_es = d3d_effect->GetVariableByName("g_light_dir_es")->AsVector();
		auto var_g_light_color = d3d_effect->GetVariableByName("g_light_color")->AsVector();

		Vector3f light_dir = TransformNormal(dir_, cam->view_);

		var_g_light_dir_es->SetFloatVector((float*)&light_dir);
		var_g_light_color->SetFloatVector((float*)&color_);
	}

}