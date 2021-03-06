#pragma once
#include <memory>

namespace zeta
{
	class DeferredRenderer;

	class Camera;
	typedef std::shared_ptr<Camera> CameraPtr;

	class Renderable;
	typedef std::shared_ptr<Renderable> RenderablePtr;

	class StaticMeshRenderable;
	typedef std::shared_ptr<StaticMeshRenderable> StaticMeshRenderablePtr;

	class QuadRenderable;
	typedef std::shared_ptr<QuadRenderable> QuadRenderablePtr;

	class SkyBoxRenderable;
	typedef std::shared_ptr<SkyBoxRenderable> SkyBoxRenderablePtr;

	class FrameBuffer;
	typedef std::shared_ptr<FrameBuffer> FrameBufferPtr;

	class AmbientLight;
	typedef std::shared_ptr<AmbientLight> AmbientLightPtr;

	class DirectionalLight;
	typedef std::shared_ptr<DirectionalLight> DirectionalLightPtr;

	class Skylight;
	typedef std::shared_ptr<Skylight> SkylightPtr;

	class SpotLight;
	typedef std::shared_ptr<SpotLight> SpotLightPtr;

	class ImageBasedProcess;
	typedef std::shared_ptr<ImageBasedProcess> ImageBasedProcessPtr;

	class OnePassPostProcess;
	typedef std::shared_ptr<OnePassPostProcess> OnePassPostProcessPtr;

	class AdaptedLumPostProcess;
	typedef std::shared_ptr<AdaptedLumPostProcess> AdaptedLumPostProcessPtr;

	class ImageStatPostProcess;
	typedef std::shared_ptr<ImageStatPostProcess> ImageStatPostProcessPtr;

	class LensEffectsPostProcess;
	typedef std::shared_ptr<LensEffectsPostProcess> LensEffectsPostProcessPtr;

	class SeparableGaussianFilterPostProcess;
	typedef std::shared_ptr<SeparableGaussianFilterPostProcess> SeparableGaussianFilterPostProcessPtr;

	class BlurPostProcess;
	typedef std::shared_ptr<BlurPostProcess> BlurPostProcessPtr;

	class HDRPostProcess;
	typedef std::shared_ptr<HDRPostProcess> HDRPostProcessPtr;

	class FXAAPostProcess;
	typedef std::shared_ptr<FXAAPostProcess> FXAAPostProcessPtr;

	class ColorGradingPostProcess;
	typedef std::shared_ptr<ColorGradingPostProcess> ColorGradingPostProcessPtr;
}
