#include "stdafx.h"
#include "Light.h"
#include "Camera.h"


namespace zeta
{

	void AmbientLight::Bind(ID3DX11Effect* d3d_effect, Camera* cam)
	{
		Vector3f light_dir(0, 1, 0);
		light_dir = TransformNormal(light_dir, cam->view_);

		SetEffectVar(d3d_effect, "g_light_dir_es", light_dir);
		SetEffectVar(d3d_effect, "g_light_color", color_);
	}


	void DirectionLight::Bind(ID3DX11Effect* d3d_effect, Camera* cam)
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