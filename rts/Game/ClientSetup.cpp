#include "ClientSetup.h"

#include "TdfParser.h"
#include "Exceptions.h"
#include "LogOutput.h"

ClientSetup::ClientSetup() :
		myPlayerNum(0),
		hostport(8452),
		sourceport(0),
		autohostport(0),
		isHost(false)
{
}

void ClientSetup::Init(const std::string& setup)
{
	TdfParser file(setup.c_str(), setup.length());

	if(!file.SectionExist("GAME"))
		throw content_error("GAME-section didn't exist in setupscript");

	// Technical parameters
	file.GetDef(hostip,     "localhost", "GAME\\HostIP");
	file.GetDef(hostport,   "8452", "GAME\\HostPort");
	file.GetDef(sourceport, "0", "GAME\\SourcePort");
	file.GetDef(autohostport, "0", "GAME\\AutohostPort");

	file.GetDef(myPlayerName,  "", "GAME\\MyPlayerName");

	if (!file.GetValue(isHost, "GAME\\IsHost"))
	{
		logOutput.Print("Warning: The script.txt is missing the IsHost-entry. Assuming this is a client.");
	}
}
