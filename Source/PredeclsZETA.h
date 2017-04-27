#pragma once
#include <memory>

namespace zeta
{
	class Renderer;

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

	class DirectionLight;
	typedef std::shared_ptr<DirectionLight> DirectionLightPtr;

	class ImageBasedProcess;
	typedef std::shared_ptr<ImageBasedProcess> ImageBasedProcessPtr;
}
