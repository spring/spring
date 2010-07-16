/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include "LobbyConnection.h"

#include <boost/bind.hpp>

#include "UpdaterWindow.h"
#include "ConfigHandler.h"
#include "Game/GameVersion.h"
#include "aGui/Gui.h"


LobbyConnection::LobbyConnection() : upwin(NULL)
{
	Connect(configHandler->GetString("LobbyServer", "taspringmaster.clan-sy.com"), 8200);
}

LobbyConnection::~LobbyConnection()
{
	if (upwin)
	{
		agui::gui->RmElement(upwin);
	}
}

void LobbyConnection::ConnectDialog(bool show)
{
	if (show && !upwin)
	{
		upwin = new UpdaterWindow(this);
		upwin->WantClose.connect(boost::bind(&LobbyConnection::ConnectDialog, this, false));
	}
	else if (!show)
	{
		agui::gui->RmElement(upwin);
		upwin = NULL;
	}
}

bool LobbyConnection::WantClose() const
{
	return upwin == NULL;
}

void LobbyConnection::DoneConnecting(bool success, const std::string& err)
{
	if (success)
	{
		upwin->ServerLabel("Connected to TASServer");
	}
	else
	{
		upwin->ServerLabel(err);
	}
}

void LobbyConnection::ServerGreeting(const std::string& serverVer, const std::string& springVer, int udpport, int mode)
{
	upwin->ServerLabel(std::string("Connected to TASServer v")+serverVer);
	if (springVer != SpringVersion::Get())
	{
		upwin->Label(std::string("Server has new version: ")+springVer + " (Yours: "+ SpringVersion::Get() + ")");
	}
	else
	{
		upwin->Label("Your version is up-to-date");
	}
}

void LobbyConnection::Denied(const std::string& reason)
{
	upwin->ServerLabel(reason);
}

void LobbyConnection::Aggreement(const std::string text)
{
	upwin->ShowAggreement(text);
}

void LobbyConnection::LoginEnd()
{
	upwin->ServerLabel("Logged in successfully");
}

void LobbyConnection::RegisterDenied(const std::string& reason)
{
	upwin->ServerLabel(reason);
}

void LobbyConnection::RegisterAccept()
{
	upwin->ServerLabel("Account registered successfully");
}
