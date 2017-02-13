/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STRING_SERIALIZER_H
#define STRING_SERIALIZER_H

#include <sstream>
#include <cstdio>

#include "Game/Players/PlayerStatistics.h"
#include "Sim/Misc/TeamStatistics.h"
#include "System/LoadSave/demofile.h"

using std::wstringstream;
using std::endl;

wstringstream& operator<<(wstringstream& str, const DemoFileHeader& header)
{
	char idbuf[128];
	const unsigned char* p = header.gameID;

	std::sprintf(idbuf,
		"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
		p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15]);

	str<<L"Magic: "<<header.magic<<endl;
	str<<L"Version: "<<header.version<<endl;
	str<<L"HeaderSize: " <<header.headerSize<<endl;
	str<<L"VersionString: " <<header.versionString<<endl;
	str<<L"GameID: " <<idbuf<<endl;
	str<<L"UnixTime: " <<header.unixTime<<endl;
	str<<L"ScriptSize: " <<header.scriptSize<<endl;
	str<<L"DemoStreamSize: " <<header.demoStreamSize<<endl;
	str<<L"GameTime: " <<header.gameTime<<endl;
	str<<L"WallclockTime: " <<header.wallclockTime<<endl;
	str<<L"NumPlayers: " <<header.numPlayers<<endl;
	str<<L"PlayerStatSize: " <<header.playerStatSize<<endl;
	str<<L"PlayerStatElemSize: " <<header.playerStatElemSize<<endl;
	str<<L"NumTeams: " <<header.numTeams<<endl;
	str<<L"TeamStatSize: " <<header.teamStatSize<<endl;
	str<<L"TeamStatElemSize: " <<header.teamStatElemSize<<endl;
	str<<L"TeamStatPeriod: " <<header.teamStatPeriod<<endl;
	str<<L"WinningAllyTeamsSize: " << header.winningAllyTeamsSize<<endl;
	return str;
}


wstringstream& operator<<(wstringstream& str, const PlayerStatistics& header)
{
	str<<L"MousePixels: " <<header.mousePixels<<endl;
	str<<L"MouseClicks: " <<header.mouseClicks<<endl;
	str<<L"KeyPresses: " <<header.keyPresses<<endl;
	str<<L"NumCommands: " <<header.numCommands<<endl;
	str<<L"UnitCommands: " <<header.unitCommands<<endl;
	return str;
}


wstringstream& operator<<(wstringstream& str, const TeamStatistics& header)
{
	str<<L"MetalUsed: " <<header.metalUsed<<endl;
	str<<L"EnergyUsed: " <<header.energyUsed<<endl;
	str<<L"MetalProduced: " <<header.metalProduced<<endl;
	str<<L"EnergyProduced: " <<header.energyProduced<<endl;
	str<<L"MetalExcess: " <<header.metalExcess<<endl;
	str<<L"EnergyExcess: " <<header.energyExcess<<endl;
	str<<L"MetalReceived: " <<header.metalReceived<<endl;
	str<<L"EnergyReceived: " <<header.energyReceived<<endl;
	str<<L"MetalSent: " <<header.metalSent<<endl;
	str<<L"EnergySent: " <<header.energySent<<endl;
	str<<L"DamageDealt: " <<header.damageDealt<<endl;
	str<<L"DamageReceived: " <<header.damageReceived<<endl;
	str<<L"UnitsProduced: " <<header.unitsProduced<<endl;
	str<<L"UnitsDied: " <<header.unitsDied<<endl;
	str<<L"UnitsReceived: " <<header.unitsReceived<<endl;
	str<<L"UnitsSent: " <<header.unitsSent<<endl;
	str<<L"UnitsCaptured: " <<header.unitsCaptured<<endl;
	str<<L"UnitsOutCaptured: " <<header.unitsOutCaptured<<endl;
	str<<L"UnitsKilled: " <<header.unitsKilled<<endl;
	return str;
}

#endif /* STRING_SERIALIZER_H */
