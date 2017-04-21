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
		void Destory();

		void Run();

	private:
		std::unique_ptr<Window> main_wnd_;
	};

}