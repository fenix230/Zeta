#pragma once
#include "Window.h"


namespace zeta
{

	class Application
	{
	public:
		Application();
		~Application();

		void Create(const std::string& name, int width, int height);

		void Run();

		DeferredRenderer& Renderer();

	private:
		std::unique_ptr<Window> main_wnd_;
		std::unique_ptr<DeferredRenderer> dr_;
	};

}