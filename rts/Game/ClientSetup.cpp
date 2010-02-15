#include "ClientSetup.h"

#include "TdfParser.h"
#include "Exceptions.h"
#include "LogOutput.h"
#include "ConfigHandler.h"
#include "Util.h"

ClientSetup::ClientSetup() :
		hostport(DEFAULT_HOST_PORT),
		sourceport(0),
		autohostip("localhost"),
		autohostport(0),
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
	file.GetDef(sourceport,   "0",                   "GAME\\SourcePort");
	file.GetDef(autohostip,   "localhost",           "GAME\\AutohostIP");
	file.GetDef(autohostport, "0",                   "GAME\\AutohostPort");

	file.GetDef(myPlayerName, "",                    "GAME\\MyPlayerName");
	file.GetDef(myPasswd, "",                    "GAME\\MyPasswd");

	if (!file.GetValue(isHost, "GAME\\IsHost")) {
		logOutput.Print("Warning: The script.txt is missing the IsHost-entry. Assuming this is a client.");
	}

	if (file.SectionExist("OPTIONS"))
	{
		TdfParser::MapRef options = file.GetAllValues("OPTIONS");
		for (std::map<std::string,std::string>::const_iterator it = options.begin(); it != options.end(); ++it)
			configHandler->SetOverlay(it->first, it->second);
	}
}
