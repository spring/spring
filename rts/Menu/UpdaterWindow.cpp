/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include "UpdaterWindow.h"

#include <boost/bind.hpp>

#include "ConfigHandler.h"
#include "lib/lobby/Connection.h"
#include "aGui/LineEdit.h"
#include "aGui/VerticalLayout.h"
#include "aGui/HorizontalLayout.h"
#include "aGui/TextElement.h"
#include "aGui/Button.h"
#include "aGui/Gui.h"

UpdaterWindow::UpdaterWindow(Connection* _con) : agui::Window("Lobby connection"), agreement(NULL), con(_con)
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
}

UpdaterWindow::~UpdaterWindow()
{
	if (agreement)
	{
		agui::gui->RmElement(agreement);
		agreement = NULL;
	}
}

void UpdaterWindow::Login()
{
	con->Login(user->GetContent(), passwd->GetContent());
	configHandler->SetString("name", user->GetContent());
}

void UpdaterWindow::Register()
{
	con->Register(user->GetContent(), passwd->GetContent());
	configHandler->SetString("name", user->GetContent());
}

void UpdaterWindow::ShowAggreement(const std::string& text)
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

void UpdaterWindow::AcceptAgreement()
{
	agui::gui->RmElement(agreement);
	agreement = NULL;
	con->ConfirmAggreement();
}

void UpdaterWindow::RejectAgreement()
{
	agui::gui->RmElement(agreement);
	agreement = NULL;
}

void UpdaterWindow::ServerLabel(const std::string& text)
{
	serverLabel->SetText(text);
}

void UpdaterWindow::Label(const std::string& text)
{
	label->SetText(text);
}
