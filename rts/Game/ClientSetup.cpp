/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ClientSetup.h"

#include "System/TdfParser.h"
#include "System/Exceptions.h"
#include "System/MsgStrings.h"
#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"
#include "System/Util.h"
#ifdef DEDICATED
#include "System/Platform/errorhandler.h"
#endif


static const unsigned int DEFAULT_HOST_PORT = 8452;
static const std::string DEFAULT_HOST_IP = "localhost"; // -> any local address
static const std::string DEFAULT_HOST_PORT_STR = IntToString(DEFAULT_HOST_PORT);

ClientSetup::ClientSetup()
	: hostIP(DEFAULT_HOST_IP)
	, hostPort(DEFAULT_HOST_PORT)
	, isHost(false)
{
}


void ClientSetup::SanityCheck()
{
	if (myPlayerName.empty()) myPlayerName = UnnamedPlayerName;
	StringReplaceInPlace(myPlayerName, ' ', '_');

	if (hostIP == "none") {
		hostIP = "";
	}
}


void ClientSetup::LoadFromStartScript(const std::string& setup)
{
	TdfParser file(setup.c_str(), setup.length());

	if (!file.SectionExist("GAME")) {
		throw content_error("GAME-section does not exist in script.txt");
	}

	// Technical parameters
	file.GetDef(hostIP,       DEFAULT_HOST_IP,       "GAME\\HostIP");
	file.GetDef(hostPort,     DEFAULT_HOST_PORT_STR, "GAME\\HostPort");

	file.GetDef(myPlayerName, "", "GAME\\MyPlayerName");
	file.GetDef(myPasswd,     "", "GAME\\MyPasswd");

	if (!file.GetValue(isHost, "GAME\\IsHost")) {
		LOG_L(L_WARNING, "script.txt is missing the IsHost-entry. Assuming this is a client.");
	}
#ifdef DEDICATED
	if (!isHost) {
		handleerror(NULL, "Error: Dedicated server needs to be host, but the start script does not have \"IsHost=1\".", "Start script error", MBF_OK|MBF_EXCL);
	}
#endif

	//FIXME WTF
	std::string sourceport, autohostip, autohostport;
	if (file.SGetValue(sourceport, "GAME\\SourcePort")) {
		configHandler->SetString("SourcePort", sourceport, true);
	}
	if (file.SGetValue(autohostip, "GAME\\AutohostIP")) {
		configHandler->SetString("AutohostIP", autohostip, true);
	}
	if (file.SGetValue(autohostport, "GAME\\AutohostPort")) {
		configHandler->SetString("AutohostPort", autohostport, true);
	}
}
