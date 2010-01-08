#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include "UpdaterWindow.h"

#include <boost/bind.hpp>

#include "Game/GameVersion.h"
#include "ConfigHandler.h"
#include "aGui/LineEdit.h"
#include "aGui/VerticalLayout.h"
#include "aGui/HorizontalLayout.h"
#include "aGui/TextElement.h"
#include "aGui/Button.h"
#include "aGui/Gui.h"

UpdaterWindow::UpdaterWindow() : agui::Window("Lobby connection"), agreement(NULL)
{
	agui::gui->AddElement(this);
	SetPos(0.5, 0.5);
	SetSize(0.4, 0.3);

	agui::VerticalLayout* wndLayout = new agui::VerticalLayout(this);
	label = new agui::TextElement("", wndLayout);

	agui::HorizontalLayout* usrLayout = new agui::HorizontalLayout(wndLayout);
	new agui::TextElement(std::string("Username:"), usrLayout);
	user = new agui::LineEdit(usrLayout);
	user->SetContent(configHandler->GetString("name", "UnnamedPlayer"));
	user->SetWeight(2);
	agui::HorizontalLayout* pwdLayout = new agui::HorizontalLayout(wndLayout);
	new agui::TextElement(std::string("Password:"), pwdLayout);
	passwd = new agui::LineEdit(pwdLayout);
	passwd->SetCrypt(true);
	passwd->SetFocus(true);
	passwd->SetWeight(2);
	
	agui::HorizontalLayout* bttnLayout = new agui::HorizontalLayout(wndLayout);
	
	agui::Button* login = new agui::Button("Login", bttnLayout);
	login->Clicked.connect(boost::bind(&UpdaterWindow::Login, this));
	agui::Button* registerb = new agui::Button("Register", bttnLayout);
	registerb->Clicked.connect(boost::bind(&UpdaterWindow::Register, this));
	agui::Button* close = new agui::Button("Close", bttnLayout);
	close->Clicked.connect(WantClose);

	serverLabel = new agui::TextElement(std::string("Connecting..."), wndLayout);

	GeometryChange();
	Connect("taspringmaster.clan-sy.com", 8200);
}

void UpdaterWindow::DoneConnecting(bool success, const std::string& err)
{
	if (success)
	{
		serverLabel->SetText("Connected to TASServer");
	}
	else
	{
		serverLabel->SetText(err);
	}
}

void UpdaterWindow::ServerGreeting(const std::string& serverVer, const std::string& springVer, int udpport, int mode)
{
	serverLabel->SetText(std::string("Connected to TASServer v")+serverVer);
	if (springVer != SpringVersion::Get())
	{
		label->SetText(std::string("Server has new version: ")+springVer + " (Yours: "+ SpringVersion::Get() + ")");
	}
	else
	{
		label->SetText("Your version is up-to-date");
	}
}

void UpdaterWindow::Denied(const std::string& reason)
{
	serverLabel->SetText(reason);
}

void UpdaterWindow::Aggreement(const std::string text)
{
	agreement = new agui::Window("Agreement");
	agreement->SetSize(0.6, 0.7);
	agui::VerticalLayout* vLay = new agui::VerticalLayout(agreement);
	agui::TextElement* textEl = new agui::TextElement(text, vLay);
	
	agui::HorizontalLayout* bttnLayout = new agui::HorizontalLayout(vLay);
	bttnLayout->SetSize(0.0f, 0.04f, true);
	agui::Button* accept = new agui::Button("I Accept", bttnLayout);
	accept->Clicked.connect(boost::bind(&UpdaterWindow::AcceptAgreement, this));
	agui::Button* noAccept = new agui::Button("I don't accept", bttnLayout);
	noAccept->Clicked.connect(boost::bind(&UpdaterWindow::RejectAgreement, this));
	agreement->WantClose.connect(boost::bind(&UpdaterWindow::RejectAgreement, this));
	agreement->GeometryChange();

	agui::gui->AddElement(agreement);
}

void UpdaterWindow::LoginEnd()
{
	serverLabel->SetText("Logged in successfully");
}

void UpdaterWindow::RegisterDenied(const std::string& reason)
{
	serverLabel->SetText(reason);
}

void UpdaterWindow::RegisterAccept()
{
	serverLabel->SetText("Account registered successfully");
}

void UpdaterWindow::Login()
{
	Connection::Login(user->GetContent(), passwd->GetContent());
	configHandler->SetString("name", user->GetContent());
}

void UpdaterWindow::Register()
{
	Connection::Register(user->GetContent(), passwd->GetContent());
	configHandler->SetString("name", user->GetContent());
}

void UpdaterWindow::AcceptAgreement()
{
	agui::gui->RmElement(agreement);
	agreement = NULL;
	AcceptAgreement();
}

void UpdaterWindow::RejectAgreement()
{
	agui::gui->RmElement(agreement);
	agreement = NULL;
}
