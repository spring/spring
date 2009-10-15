#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include "UpdaterWindow.h"

#include <boost/bind.hpp>

#include "aGui/VerticalLayout.h"
#include "aGui/HorizontalLayout.h"
#include "aGui/TextElement.h"
#include "aGui/Button.h"
#include "aGui/Gui.h"

UpdaterWindow::UpdaterWindow() : agui::Window("Update checker")
{
	agui::gui->AddElement(this);
	SetPos(0.5, 0.5);
	SetSize(0.4, 0.2);
	
	agui::VerticalLayout* wndLayout = new agui::VerticalLayout(this);
	new agui::TextElement(std::string("Spring version: ")+SpringVersion::Get(), wndLayout);
	label = new agui::TextElement("Connecting...", wndLayout);
	close = new agui::Button("Close", wndLayout);
	close->Clicked.connect(WantClose);
	GeometryChange();
	Connect("taspringmaster.clan-sy.com", 8200);
}

void UpdaterWindow::DoneConnecting(bool success, const std::string& err)
{
	if (success)
	{
		label->SetText("Connected");
	}
	else
	{
		label->SetText(err);
	}
}

void UpdaterWindow::DataReceived(const std::string& command, const std::string& msg)
{
	if (command == "TASServer")
	{
		if (!msg.empty())
		{
			RawTextMessage buf(msg);
			std::string serverver, clientver, udpport, mode;
			serverver = buf.GetWord();
			clientver = buf.GetWord();
			udpport = buf.GetWord();
			mode = buf.GetWord();
			if (clientver != SpringVersion::Get())
			{
				label->SetText(std::string("Server has new version: ")+clientver);
				close->Label("Damn!");
			}
			else
			{
				label->SetText("Your version is up-to-date");
				close->Label("Good!");
			}
		}
	}
}
