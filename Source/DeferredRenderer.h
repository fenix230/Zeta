#pragma once
#include "Utils.h"


namespace zeta
{

	class DeferredRenderer
	{
	public:
		DeferredRenderer();
		~DeferredRenderer();

		void Create(HWND wnd, uint32_t width, uint32_t height);

		void Resize(uint32_t width, uint32_t height);

		void SetCamera(CameraPtr cam);
		CameraPtr GetCamera() const;

		void SetSkyBox(SkyBoxRenderablePtr skybox);

		void AddRenderable(RenderablePtr r);

		void SetAmbientLight(AmbientLightPtr l);
		void SetSkyLight(SkylightPtr l);
		void AddDirectionLight(DirectionalLightPtr l);
		void AddSpotLight(SpotLightPtr l);

		void Frame();

		void UpdateStat();

	private:
		FrameBufferPtr gbuffer_fb_;
		FrameBufferPtr linear_depth_fb_;
		FrameBufferPtr lighting_fb_;
		FrameBufferPtr shading_fb_;
		FrameBufferPtr hdr_fb_;
		FrameBufferPtr fxaa_fb_;
		FrameBufferPtr color_grading_fb_;

		HDRPostProcessPtr hdr_pp_;
		FXAAPostProcessPtr fxaa_pp_;
		ColorGradingPostProcessPtr color_grading_pp_;

		QuadRenderablePtr quad_; 

		CameraPtr cam_;
		SkyBoxRenderablePtr skybox_;
		std::vector<RenderablePtr> rs_;

		AmbientLightPtr ambient_light_;
		SkylightPtr skylight_;
		std::vector<DirectionalLightPtr> dir_lights_;
		std::vector<SpotLightPtr> spot_lights_;

		Timer timer_;
	};

}