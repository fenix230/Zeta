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


	class DirectionLight
	{
	public:
		void Bind(ID3DX11Effect* d3d_effect, Camera* cam);

		Vector3f dir_;
		Vector3f color_;
	};

}