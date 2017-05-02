#include "stdafx.h"
#include "Application.h"
#include "Renderer.h"


namespace zeta
{

	Application::Application()
	{

	}

	Application::~Application()
	{
	}

	void Application::Create(const std::string& name, int width, int height)
	{
		main_wnd_ = std::make_unique<Window>(name, width, height);
		Renderer::Instance().Create(main_wnd_->HWnd(), width, height);
	}

	void Application::Run()
	{
		bool gotMsg;
		MSG  msg;

		::PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);

		while (WM_QUIT != msg.message)
		{
			if (main_wnd_ && main_wnd_->Active())
			{
				gotMsg = (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0);
			}
			else
			{
				gotMsg = (::GetMessage(&msg, nullptr, 0, 0) != 0);
			}

			if (gotMsg)
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
			else
			{
				Renderer::Instance().Frame();
			}
		}
	}

}