/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ClientSetup.h"

#include "System/TdfParser.h"
#include "System/Exceptions.h"
#include "System/MsgStrings.h"
#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"
#include "System/StringUtil.h"
#ifdef DEDICATED
#include "System/Platform/errorhandler.h"
#endif


CONFIG(std::string, HostIPDefault).defaultValue("localhost").dedicatedValue("").description("Default IP to use for hosting if not specified in script.txt");
CONFIG(int, HostPortDefault).defaultValue(8452).minimumValue(0).maximumValue(65535).description("Default Port to use for hosting if not specified in script.txt");

ClientSetup::ClientSetup()
	: hostIP(configHandler->GetString("HostIPDefault"))
	, hostPort(configHandler->GetInt("HostPortDefault"))
	, isHost(false)
{
}


void ClientSetup::SanityCheck()
{
	if (myPlayerName.empty())
		myPlayerName = UnnamedPlayerName;

	StringReplaceInPlace(myPlayerName, ' ', '_');
}


void ClientSetup::LoadFromStartScript(const std::string& setup)
{
	TdfParser file(setup.c_str(), setup.length());

	if (!file.SectionExist("GAME")) {
		LOG_L(L_ERROR, "[ClientSetup::%s] GAME-section missing from setup-script \"%s\"", __func__, setup.c_str());
		throw content_error("setup-script error");
	}

	// Technical parameters
	file.GetDef(hostIP,       hostIP, "GAME\\HostIP");
	file.GetDef(hostPort,     IntToString(hostPort), "GAME\\HostPort");

	file.GetDef(myPlayerName, "", "GAME\\MyPlayerName");
	file.GetDef(myPasswd,     "", "GAME\\MyPasswd");

	if (!file.GetValue(isHost, "GAME\\IsHost"))
		LOG_L(L_WARNING, "[ClientSetup::%s] IsHost-entry missing from setup-script; assuming this is a client", __func__);

#ifdef DEDICATED
	if (!isHost)
		handleerror(nullptr, "setup-script error", "dedicated server needs \"IsHost=1\" in GAME-section", MBF_OK | MBF_EXCL);
#endif

	// FIXME WTF
	std::string sourceport;
	std::string autohostip;
	std::string autohostport;

	if (file.SGetValue(sourceport, "GAME\\SourcePort"))
		configHandler->SetString("SourcePort", sourceport, true);

	if (file.SGetValue(autohostip, "GAME\\AutohostIP"))
		configHandler->SetString("AutohostIP", autohostip, true);

	if (file.SGetValue(autohostport, "GAME\\AutohostPort"))
		configHandler->SetString("AutohostPort", autohostport, true);

	file.GetDef(saveFile, "", "GAME\\SaveFile");
	file.GetDef(demoFile, "", "GAME\\DemoFile");
}
