/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BaseNetProtocol.h"

#include "Game/Players/PlayerStatistics.h"
#include "Sim/Misc/TeamStatistics.h"
#include "System/Net/RawPacket.h"
#include "System/Net/PackPacket.h"
#include "System/Net/ProtocolDef.h"
#include <cinttypes>

using netcode::PackPacket;
typedef std::shared_ptr<const netcode::RawPacket> PacketType;

CBaseNetProtocol& CBaseNetProtocol::Get()
{
	static CBaseNetProtocol instance;
	return instance;
}

PacketType CBaseNetProtocol::SendKeyFrame(int32_t frameNum)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(frameNum), NETMSG_KEYFRAME);
	*packet << frameNum;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendNewFrame()
{
	return PacketType(new PackPacket(sizeof(uint8_t), NETMSG_NEWFRAME));
}


PacketType CBaseNetProtocol::SendQuit(const std::string& reason)
{
	// NOTE:
	//   transmit size as uint16, but calculate it using uint32's so overflow is safe
	//   the extra null-terminator +1 is necessary, PackPacket adds a byte for strings
	// const uint32_t headerSize = sizeof(static_cast<uint8_t>(NETMSG_QUIT)) + sizeof(static_cast<uint16_t>(packetSize));
	const uint32_t payloadSize = reason.size() + 1;
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_QUIT);
	*packet << static_cast<uint16_t>(packetSize) << reason;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendStartPlaying(uint32_t countdown)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(countdown), NETMSG_STARTPLAYING);
	*packet << countdown;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSetPlayerNum(uint8_t playerNum)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum), NETMSG_SETPLAYERNUM);
	*packet << playerNum;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendPlayerName(uint8_t playerNum, const std::string& playerName)
{
	const uint32_t payloadSize = sizeof(playerNum) + (playerName.size() + 1);
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint8_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_PLAYERNAME);
	*packet << static_cast<uint8_t>(packetSize) << playerNum << playerName;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendRandSeed(uint32_t randSeed)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(randSeed), NETMSG_RANDSEED);
	*packet << randSeed;
	return PacketType(packet);
}

// NETMSG_GAMEID = 9, char gameID[16];
PacketType CBaseNetProtocol::SendGameID(const uint8_t* buf)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + 16, NETMSG_GAMEID);
	memcpy(packet->GetWritingPos(), buf, 16);
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendPathCheckSum(uint8_t playerNum, uint32_t checksum)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(uint32_t), NETMSG_PATH_CHECKSUM);
	*packet << playerNum;
	*packet << checksum;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendSelect(uint8_t playerNum, const std::vector<int16_t>& selectedUnitIDs)
{
	const uint32_t payloadSize = sizeof(playerNum) + (selectedUnitIDs.size() * sizeof(int16_t));
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_SELECT);
	*packet << static_cast<uint16_t>(packetSize) << playerNum << selectedUnitIDs;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendPause(uint8_t playerNum, uint8_t bPaused)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(bPaused), NETMSG_PAUSE);
	*packet << playerNum << bPaused;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendCommand(
	uint8_t playerNum,
	int32_t commandID,
	int32_t timeout,
	uint8_t options,
	uint32_t numParams,
	const float* params
) {
	const uint32_t payloadSize = sizeof(playerNum) + sizeof(commandID) + sizeof(timeout) + sizeof(options) + sizeof(numParams) + (numParams * sizeof(float));
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_COMMAND);
	*packet << static_cast<uint16_t>(packetSize) << playerNum << commandID << timeout << options << numParams;

	for (uint32_t i = 0; i < numParams; i++) {
		*packet << params[i];
	}

	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendAICommand(
	uint8_t playerNum,
	uint8_t aiInstID,
	uint8_t aiTeamID,
	int16_t unitID,
	int32_t commandID,
	int32_t aiCommandID,
	int32_t timeout,
	uint8_t options,
	uint32_t numParams,
	const float* params
) {
	const int32_t commandTypeID = (aiCommandID != -1)? NETMSG_AICOMMAND_TRACKED: NETMSG_AICOMMAND;

	const uint32_t payloadSize =
		sizeof(playerNum) + sizeof(aiInstID) + sizeof(aiTeamID) + sizeof(unitID) +
		sizeof(commandID) + sizeof(timeout) + sizeof(options) + sizeof(numParams) +
		(sizeof(commandTypeID) * (commandTypeID == NETMSG_AICOMMAND_TRACKED)) + (numParams * sizeof(float));
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	// packetSize must be an uint32_t for this to work
	if (packetSize >= (1 << (sizeof(uint16_t) * 8)))
		throw netcode::PackPacketException("[BaseNetProto::SendAICommand] maximum packet-size exceeded");

	PackPacket* packet = new PackPacket(packetSize, commandTypeID);
	*packet << static_cast<uint16_t>(packetSize) << playerNum << aiInstID << aiTeamID << unitID;
	*packet << commandID << timeout << options << numParams;

	if (commandTypeID == NETMSG_AICOMMAND_TRACKED)
		*packet << aiCommandID;

	for (uint32_t i = 0; i < numParams; i++) {
		*packet << params[i];
	}

	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendAIShare(
	uint8_t playerNum,
	uint8_t aiID,
	uint8_t sourceTeam,
	uint8_t destTeam,
	float metal,
	float energy,
	const std::vector<int16_t>& unitIDs
) {
	const uint32_t payloadSize = sizeof(playerNum) + sizeof(aiID) + sizeof(sourceTeam) + sizeof(destTeam) + (2 * sizeof(float)) + (unitIDs.size() * sizeof(int16_t));
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	if (packetSize >= (1 << (sizeof(uint16_t) * 8)))
		throw netcode::PackPacketException("[BaseNetProto::SendAIShare] maximum packet-size exceeded");

	PackPacket* packet = new PackPacket(packetSize, NETMSG_AISHARE);
	*packet << static_cast<uint16_t>(packetSize) << playerNum << aiID << sourceTeam << destTeam << metal << energy << unitIDs;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendUserSpeed(uint8_t playerNum, float userSpeed)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(userSpeed), NETMSG_USER_SPEED);
	*packet << playerNum << userSpeed;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendInternalSpeed(float internalSpeed)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(internalSpeed), NETMSG_INTERNAL_SPEED);
	*packet << internalSpeed;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendCPUUsage(float cpuUsage)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(cpuUsage), NETMSG_CPU_USAGE);
	*packet << cpuUsage;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendDirectControl(uint8_t playerNum)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum), NETMSG_DIRECT_CONTROL);
	*packet << playerNum;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendDirectControlUpdate(uint8_t playerNum, uint8_t status, int16_t heading, int16_t pitch)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(status) + sizeof(heading) + sizeof(pitch), NETMSG_DC_UPDATE);
	*packet << playerNum << status << heading << pitch;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendAttemptConnect(
	const std::string& name,
	const std::string& passwd,
	const std::string& version,
	const std::string& platform,
	int32_t netloss,
	bool reconnect
) {
	const uint32_t payloadSize =
		sizeof(NETWORK_VERSION) +
		sizeof(static_cast<uint8_t>(netloss)) +
		sizeof(static_cast<uint8_t>(reconnect)) +
		(name.size() + 1) +
		(passwd.size() + 1) +
		(version.size() + 1) +
		(platform.size() + 1);
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_ATTEMPTCONNECT);
	*packet << static_cast<uint16_t>(packetSize);
	*packet << NETWORK_VERSION;
	*packet << name;
	*packet << passwd;
	*packet << version;
	*packet << platform;
	*packet << uint8_t(reconnect);
	*packet << uint8_t(netloss);

	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendRejectConnect(const std::string& reason)
{
	const uint32_t payloadSize = reason.size() + 1;
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_REJECT_CONNECT);
	*packet << static_cast<uint16_t>(packetSize) << reason;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendShare(uint8_t playerNum, uint8_t shareTeam, uint8_t bShareUnits, float shareMetal, float shareEnergy)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(shareTeam) + sizeof(bShareUnits) + (sizeof(shareMetal) * 2), NETMSG_SHARE);
	*packet << playerNum << shareTeam << bShareUnits << shareMetal << shareEnergy;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSetShare(uint8_t playerNum, uint8_t myTeam, float metalShareFraction, float energyShareFraction)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(myTeam) + (sizeof(metalShareFraction) * 2), NETMSG_SETSHARE);
	*packet << playerNum << myTeam << metalShareFraction << energyShareFraction;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendPlayerStat(uint8_t playerNum, const PlayerStatistics& currentStats)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(PlayerStatistics), NETMSG_PLAYERSTAT);
	*packet << playerNum << currentStats;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendTeamStat(uint8_t teamNum, const TeamStatistics& currentStats)
{
	PackPacket* packet = new netcode::PackPacket(sizeof(uint8_t) + sizeof(teamNum) + sizeof(TeamStatistics), NETMSG_TEAMSTAT);
	*packet << teamNum << currentStats;
	return PacketType(packet);
}



PacketType CBaseNetProtocol::SendGameOver(uint8_t playerNum, const std::vector<uint8_t>& winningAllyTeams)
{
	const uint32_t payloadSize = sizeof(playerNum) + (winningAllyTeams.size() * sizeof(uint8_t));
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint8_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_GAMEOVER);
	*packet << static_cast<uint8_t>(packetSize) << playerNum << winningAllyTeams;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendMapErase(uint8_t playerNum, int16_t x, int16_t z)
{
	constexpr uint8_t drawType = MAPDRAW_ERASE;

	const uint32_t payloadSize = sizeof(playerNum) + sizeof(drawType) + sizeof(x) + sizeof(z);
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint8_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_MAPDRAW);
	*packet << static_cast<uint8_t>(packetSize) << playerNum << drawType << x << z;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendMapDrawPoint(uint8_t playerNum, int16_t x, int16_t z, const std::string& label, bool fromLua)
{
	constexpr uint8_t drawType = MAPDRAW_POINT;

	const uint32_t payloadSize = sizeof(playerNum) + sizeof(drawType) + sizeof(x) + sizeof(z) + sizeof(fromLua) + (label.size() + 1);
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint8_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_MAPDRAW);
	*packet <<
		static_cast<uint8_t>(packetSize) <<
		playerNum <<
		drawType <<
		x <<
		z <<
		static_cast<uint8_t>(fromLua) <<
		label;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendMapDrawLine(uint8_t playerNum, int16_t x1, int16_t z1, int16_t x2, int16_t z2, bool fromLua)
{
	constexpr uint8_t drawType = MAPDRAW_LINE;

	const uint32_t payloadSize = sizeof(playerNum) + sizeof(drawType) + sizeof(x1) + sizeof(z1) + sizeof(x2) + sizeof(z2) + sizeof(fromLua);
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint8_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_MAPDRAW);
	*packet <<
		static_cast<uint8_t>(packetSize) <<
		playerNum <<
		drawType <<
		x1 << z1 <<
		x2 << z2 <<
		static_cast<uint8_t>(fromLua);
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendSyncResponse(uint8_t playerNum, int32_t frameNum, uint32_t checksum)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(frameNum) + sizeof(checksum), NETMSG_SYNCRESPONSE);
	*packet << playerNum << frameNum << checksum;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSystemMessage(uint8_t playerNum, std::string message)
{
	if (message.size() > 65000) {
		message.resize(65000);
		message.append("...");
	}

	const uint32_t payloadSize = sizeof(playerNum) + (message.size() + 1);
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_SYSTEMMSG);
	*packet << static_cast<uint16_t>(packetSize) << playerNum << message;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendStartPos(uint8_t playerNum, uint8_t teamNum, uint8_t readyState, float x, float y, float z)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(teamNum) + sizeof(readyState) + (3 * sizeof(x)), NETMSG_STARTPOS);
	*packet << playerNum << teamNum << readyState << x << y << z;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendPlayerInfo(uint8_t playerNum, float cpuUsage, int32_t ping)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(cpuUsage) + sizeof(ping), NETMSG_PLAYERINFO);
	*packet << playerNum << cpuUsage << static_cast<uint32_t>(ping);
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendPlayerLeft(uint8_t playerNum, uint8_t bIntended)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(bIntended), NETMSG_PLAYERLEFT);
	*packet << playerNum << bIntended;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendLogMsg(uint8_t playerNum, uint8_t logMsgLvl, const std::string& strData)
{
	const uint32_t payloadSize = sizeof(playerNum) + sizeof(logMsgLvl) + (strData.size() + 1);
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	if (packetSize >= (1 << (sizeof(uint16_t) * 8)))
		throw netcode::PackPacketException("[BaseNetProto::SendLogMsg] maximum packet-size exceeded");

	PackPacket* packet = new PackPacket(packetSize, NETMSG_LOGMSG);
	*packet << static_cast<uint16_t>(packetSize) << playerNum << logMsgLvl << strData;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendLuaMsg(uint8_t playerNum, uint16_t script, uint8_t mode, const std::vector<uint8_t>& rawData)
{
	const uint32_t payloadSize = sizeof(playerNum) + sizeof(script) + sizeof(mode) + rawData.size();
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	if (packetSize >= (1 << (sizeof(uint16_t) * 8)))
		throw netcode::PackPacketException("[BaseNetProto::SendLuaMsg] maximum packet-size exceeded");

	PackPacket* packet = new PackPacket(packetSize, NETMSG_LUAMSG);
	*packet << static_cast<uint16_t>(packetSize) << playerNum << script << mode << rawData;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendGiveAwayEverything(uint8_t playerNum, uint8_t giveToTeam, uint8_t takeFromTeam)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + 1 + sizeof(giveToTeam) + sizeof(takeFromTeam), NETMSG_TEAM);
	*packet << playerNum << static_cast<uint8_t>(TEAMMSG_GIVEAWAY) << giveToTeam << takeFromTeam;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendResign(uint8_t playerNum)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + 1 + 1 + 1, NETMSG_TEAM);
	*packet << playerNum << static_cast<uint8_t>(TEAMMSG_RESIGN) << static_cast<uint8_t>(0) << static_cast<uint8_t>(0);
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendJoinTeam(uint8_t playerNum, uint8_t wantedTeamNum)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + 1 + sizeof(wantedTeamNum) + 1, NETMSG_TEAM);
	*packet << playerNum << static_cast<uint8_t>(TEAMMSG_JOIN_TEAM) << wantedTeamNum << static_cast<uint8_t>(0);
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendTeamDied(uint8_t playerNum, uint8_t whichTeam)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + 1 + sizeof(whichTeam) + 1, NETMSG_TEAM);
	*packet << playerNum << static_cast<uint8_t>(TEAMMSG_TEAM_DIED) << whichTeam << static_cast<uint8_t>(0);
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendAICreated(uint8_t playerNum, uint8_t whichSkirmishAI, uint8_t team, const std::string& name)
{
	// do not hand optimize this math; the compiler will do that
	const uint32_t payloadSize = sizeof(playerNum) + sizeof(whichSkirmishAI) + sizeof(team) + (name.size() + 1);
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint8_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_AI_CREATED);
	*packet
		<< static_cast<uint8_t>(packetSize)
		<< playerNum
		<< whichSkirmishAI
		<< team
		<< name;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendAIStateChanged(uint8_t playerNum, uint8_t whichSkirmishAI, uint8_t newState)
{
	// do not hand optimize this math; the compiler will do that
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(whichSkirmishAI) + sizeof(newState), NETMSG_AI_STATE_CHANGED);
	*packet << playerNum << whichSkirmishAI << newState;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSetAllied(uint8_t playerNum, uint8_t whichAllyTeam, uint8_t state)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(playerNum) + sizeof(whichAllyTeam) + sizeof(state), NETMSG_ALLIANCE);
	*packet << playerNum << whichAllyTeam << state;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendCreateNewPlayer(uint8_t playerNum, bool spectator, uint8_t teamNum, const std::string& playerName )
{
	const uint32_t payloadSize = sizeof(playerNum) + sizeof(spectator) + sizeof(teamNum) + (playerName.size() + 1);
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_CREATE_NEWPLAYER);
	*packet << static_cast<uint16_t>(packetSize) << playerNum << (uint8_t)spectator << teamNum << playerName;
	return PacketType(packet);

}

PacketType CBaseNetProtocol::SendCurrentFrameProgress(int32_t frameNum)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(frameNum), NETMSG_GAME_FRAME_PROGRESS);
	*packet << frameNum;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendPing(uint8_t playerNum, uint8_t pingTag, float localTime)
{
	const uint32_t payloadSize = sizeof(playerNum) + sizeof(pingTag) + sizeof(localTime);
	const uint32_t headerSize = sizeof(uint8_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_PING);
	*packet << playerNum;
	*packet << pingTag;
	*packet << localTime;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendClientData(uint8_t playerNum, const std::vector<uint8_t>& data)
{
	const uint32_t payloadSize = sizeof(playerNum) + data.size();
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_CLIENTDATA);
	*packet << static_cast<uint16_t>(packetSize);
	*packet << playerNum;
	*packet << data;

	return PacketType(packet);
}



#ifdef SYNCDEBUG
PacketType CBaseNetProtocol::SendSdCheckrequest(int32_t frameNum)
{
	PackPacket* packet = new PackPacket(5, NETMSG_SD_CHKREQUEST);
	*packet << frameNum;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSdCheckresponse(uint8_t playerNum, uint64_t flop, std::vector<uint32_t> checksums)
{
	const uint32_t payloadSize = sizeof(playerNum) + sizeof(flop) + (checksums.size() * sizeof(uint32_t));
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_SD_CHKRESPONSE);
	*packet << static_cast<uint16_t>(packetSize) << playerNum << flop << checksums;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSdReset()
{
	return PacketType(new PackPacket(sizeof(uint8_t), NETMSG_SD_RESET));
}


PacketType CBaseNetProtocol::SendSdBlockrequest(uint16_t begin, uint16_t length, uint16_t requestSize)
{
	PackPacket* packet = new PackPacket(sizeof(uint8_t) + sizeof(begin) + sizeof(length) + sizeof(requestSize), NETMSG_SD_BLKREQUEST);
	*packet << begin << length << requestSize;
	return PacketType(packet);

}

PacketType CBaseNetProtocol::SendSdBlockresponse(uint8_t playerNum, std::vector<uint32_t> checksums)
{
	const uint32_t payloadSize = sizeof(playerNum) + (checksums.size() * sizeof(uint32_t));
	const uint32_t headerSize = sizeof(uint8_t) + sizeof(uint16_t);
	const uint32_t packetSize = headerSize + payloadSize;

	PackPacket* packet = new PackPacket(packetSize, NETMSG_SD_BLKRESPONSE);
	*packet << static_cast<uint16_t>(packetSize) << playerNum << checksums;
	return PacketType(packet);
}
#endif // SYNCDEBUG


CBaseNetProtocol::CBaseNetProtocol()
{
	netcode::ProtocolDef* proto = netcode::ProtocolDef::GetInstance();
	// proto->AddType() length parameter:
	//   > 0:  if its fixed length
	//   < 0:  means the next x bytes represent the length

	proto->AddType(NETMSG_KEYFRAME, 5);
	proto->AddType(NETMSG_NEWFRAME, 1);
	proto->AddType(NETMSG_QUIT, -2);
	proto->AddType(NETMSG_STARTPLAYING, 5);
	proto->AddType(NETMSG_SETPLAYERNUM, 2);
	proto->AddType(NETMSG_PLAYERNAME, -1);
	proto->AddType(NETMSG_CHAT, -1);
	proto->AddType(NETMSG_RANDSEED, 5);
	proto->AddType(NETMSG_GAMEID, 17);
	proto->AddType(NETMSG_PATH_CHECKSUM, 1 + 1 + sizeof(uint32_t));
	proto->AddType(NETMSG_COMMAND, -2);
	proto->AddType(NETMSG_SELECT, -2);
	proto->AddType(NETMSG_PAUSE, 3);

	proto->AddType(NETMSG_AICOMMAND, -2);
	proto->AddType(NETMSG_AICOMMAND_TRACKED, -2);
	proto->AddType(NETMSG_AICOMMANDS, -2);
	proto->AddType(NETMSG_AISHARE, -2);

	proto->AddType(NETMSG_USER_SPEED, 6);
	proto->AddType(NETMSG_INTERNAL_SPEED, 5);
	proto->AddType(NETMSG_CPU_USAGE, 5);
	proto->AddType(NETMSG_DIRECT_CONTROL, 2);
	proto->AddType(NETMSG_DC_UPDATE, 7);
	proto->AddType(NETMSG_ATTEMPTCONNECT, -2);
	proto->AddType(NETMSG_REJECT_CONNECT, -2);
	proto->AddType(NETMSG_SHARE, 12);
	proto->AddType(NETMSG_SETSHARE, 11);

	proto->AddType(NETMSG_PLAYERSTAT, 2 + sizeof(PlayerStatistics));
	proto->AddType(NETMSG_GAMEOVER, -1);
	proto->AddType(NETMSG_MAPDRAW, -1);
	proto->AddType(NETMSG_SYNCRESPONSE, 10);
	proto->AddType(NETMSG_SYSTEMMSG, -2);
	proto->AddType(NETMSG_STARTPOS, 16);
	proto->AddType(NETMSG_PLAYERINFO, 10);
	proto->AddType(NETMSG_PLAYERLEFT, 3);
	proto->AddType(NETMSG_LOGMSG, -2);
	proto->AddType(NETMSG_LUAMSG, -2);
	proto->AddType(NETMSG_TEAM, 5);
	proto->AddType(NETMSG_GAMEDATA, -2);
	proto->AddType(NETMSG_ALLIANCE, 4);
	proto->AddType(NETMSG_CCOMMAND, -2);
	proto->AddType(NETMSG_TEAMSTAT, 2 + sizeof(TeamStatistics));
	proto->AddType(NETMSG_CLIENTDATA, -2);
	proto->AddType(NETMSG_REQUEST_TEAMSTAT, 4 );

	proto->AddType(NETMSG_CREATE_NEWPLAYER, -2);

	proto->AddType(NETMSG_AI_CREATED, -1);
	proto->AddType(NETMSG_AI_STATE_CHANGED, 4);
	proto->AddType(NETMSG_GAME_FRAME_PROGRESS, 5);
	proto->AddType(NETMSG_PING, 1 + (1 + 1 + 4));

#ifdef SYNCDEBUG
	proto->AddType(NETMSG_SD_CHKREQUEST, 5);
	proto->AddType(NETMSG_SD_CHKRESPONSE, -2);
	proto->AddType(NETMSG_SD_RESET, 1);
	proto->AddType(NETMSG_SD_BLKREQUEST, 7);
	proto->AddType(NETMSG_SD_BLKRESPONSE, -2);
#endif // SYNCDEBUG
}

