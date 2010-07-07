/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ClientSetup.h"

#include "TdfParser.h"
#include "Exceptions.h"
#include "LogOutput.h"
#include "ConfigHandler.h"
#include "Util.h"
#ifdef DEDICATED
#include "Platform/errorhandler.h"
#endif

ClientSetup::ClientSetup() :
		hostport(DEFAULT_HOST_PORT),
		isHost(false)
{
}

void ClientSetup::Init(const std::string& setup)
{
	static const std::string DEFAULT_HOST_PORT_STR = IntToString(DEFAULT_HOST_PORT);

	TdfParser file(setup.c_str(), setup.length());

	if (!file.SectionExist("GAME"))
	{
		throw content_error("GAME-section did not exist in setupscript");
	}

	// Technical parameters
	file.GetDef(hostip,       "localhost",           "GAME\\HostIP");
	file.GetDef(hostport,     DEFAULT_HOST_PORT_STR, "GAME\\HostPort");

	file.GetDef(myPlayerName, "",                    "GAME\\MyPlayerName");
	file.GetDef(myPasswd, "",                    "GAME\\MyPasswd");

	if (!file.GetValue(isHost, "GAME\\IsHost")) {
		logOutput.Print("Warning: The script.txt is missing the IsHost-entry. Assuming this is a client.");
	}
#ifdef DEDICATED
	if (!isHost) {
		handleerror(NULL, "Error: Dedicated server needs to be host, but the start script does not have \"IsHost=1\".", "Start script error", MBF_OK|MBF_EXCL);
	}
#endif

	std::string sourceport, autohostip, autohostport;
	if (file.SGetValue(sourceport, "GAME\\SourcePort"))
		configHandler->SetOverlay("SourcePort", sourceport);
	if (file.SGetValue(autohostip, "GAME\\AutohostIP"))
		configHandler->SetOverlay("AutohostIP", autohostip);
	if (file.SGetValue(autohostport, "GAME\\AutohostPort"))
		configHandler->SetOverlay("AutohostPort", autohostport);

	if (file.SectionExist("OPTIONS"))
	{
		TdfParser::MapRef options = file.GetAllValues("OPTIONS");
		for (std::map<std::string,std::string>::const_iterator it = options.begin(); it != options.end(); ++it)
			configHandler->SetOverlay(it->first, it->second);
	}
}
