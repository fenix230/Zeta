#include "stdafx.h"
#include "Camera.h"


namespace zeta
{

	void Camera::Bind(ID3DX11Effect* effect)
	{
		Matrix inv_proj = Inverse(proj_);

		SetEffectVar(effect, "g_model_mat", world_);
		SetEffectVar(effect, "g_view_mat", view_);
		SetEffectVar(effect, "g_proj_mat", proj_);
		SetEffectVar(effect, "g_inv_proj_mat", inv_proj);
	}

	void Camera::LookAt(Vector3f pos, Vector3f target, Vector3f up)
	{
		view_ = XMMatrixLookAtLH(pos.XMV(), target.XMV(), up.XMV());
		eye_pos_ = pos;
		look_at_ = target;
		up_ = up;
	}

	void Camera::Perspective(float ang, float aspect, float near_plane, float far_plane)
	{
		proj_ = XMMatrixPerspectiveFovLH(ang, aspect, near_plane, far_plane);

		ang_ = ang;
		aspect_ = aspect;
		near_plane_ = near_plane;
		far_plane_ = far_plane;
	}

	Vector3f Camera::ForwardVec()
	{
		return Normalize(look_at_ - eye_pos_);
	}

}