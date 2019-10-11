/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASE_NET_PROTOCOL_H
#define _BASE_NET_PROTOCOL_H

#include <cinttypes>
#include <cstdlib>
#include <memory>
#include <vector>
#include <string>

#include "Game/GameVersion.h"
#include "NetMessageTypes.h"

#if (!defined(DEDICATED) && !defined(UNITSYNC) && !defined(BUILDING_AI) && !defined(UNIT_TEST))
#define CLIENT_NETLOG(p, l, m) clientNet->Send(CBaseNetProtocol::Get().SendLogMsg((p), (l), (m)))
#else
#define CLIENT_NETLOG(p, l, m)
#endif

namespace netcode
{
	class RawPacket;
}
struct PlayerStatistics;
struct TeamStatistics;


static const uint16_t NETWORK_VERSION = atoi(SpringVersion::GetMajor().c_str());


/**
 * @brief A factory used to make often-used network messages.
 *
 * Use this if you want to create a network message. Implemented as a singleton.
 */
class CBaseNetProtocol
{
public:
	typedef std::shared_ptr<const netcode::RawPacket> PacketType;

	static CBaseNetProtocol& Get();

	PacketType SendKeyFrame(int32_t frameNum);
	PacketType SendNewFrame();
	PacketType SendQuit(const std::string& reason);
	/// client can send these to force-start the game
	PacketType SendStartPlaying(uint32_t countdown);
	PacketType SendSetPlayerNum(uint8_t playerNum);
	PacketType SendPlayerName(uint8_t playerNum, const std::string& playerName);
	PacketType SendRandSeed(uint32_t randSeed);
	PacketType SendGameID(const uint8_t* buf);
	PacketType SendPathCheckSum(uint8_t playerNum, uint32_t checksum);
	PacketType SendSelect(uint8_t playerNum, const std::vector<int16_t>& selectedUnitIDs);
	PacketType SendPause(uint8_t playerNum, uint8_t bPaused);

	PacketType SendCommand(uint8_t playerNum, int32_t commandID, int32_t timeout, uint8_t options, uint32_t numParams, const float* params);
	PacketType SendAICommand(uint8_t playerNum, uint8_t aiInstID, uint8_t aiTeamID, int16_t unitID, int32_t commandID, int32_t aiCommandID, int32_t timeout, uint8_t options, uint32_t numParams, const float* params);
	PacketType SendAIShare(uint8_t playerNum, uint8_t aiID, uint8_t sourceTeam, uint8_t destTeam, float metal, float energy, const std::vector<int16_t>& unitIDs);

	PacketType SendUserSpeed(uint8_t playerNum, float userSpeed);
	PacketType SendInternalSpeed(float internalSpeed);
	PacketType SendCPUUsage(float cpuUsage);
	PacketType SendCustomData(uint8_t playerNum, uint8_t dataType, int32_t dataValue);
	PacketType SendLuaDrawTime(uint8_t playerNum, int32_t mSec);
	PacketType SendDirectControl(uint8_t playerNum);
	PacketType SendDirectControlUpdate(uint8_t playerNum, uint8_t status, int16_t heading, int16_t pitch);
	PacketType SendAttemptConnect(const std::string& name, const std::string& passwd, const std::string& version, const std::string& platform, int32_t netloss, bool reconnect = false);
	PacketType SendRejectConnect(const std::string& reason);
	PacketType SendShare(uint8_t playerNum, uint8_t shareTeam, uint8_t bShareUnits, float shareMetal, float shareEnergy);
	PacketType SendSetShare(uint8_t playerNum, uint8_t myTeam, float metalShareFraction, float energyShareFraction);
	PacketType SendGameOver(uint8_t playerNum, const std::vector<uint8_t>& winningAllyTeams);
	PacketType SendMapErase(uint8_t playerNum, int16_t x, int16_t z);
	PacketType SendMapDrawLine(uint8_t playerNum, int16_t x1, int16_t z1, int16_t x2, int16_t z2, bool);
	PacketType SendMapDrawPoint(uint8_t playerNum, int16_t x, int16_t z, const std::string& label, bool);
	PacketType SendSyncResponse(uint8_t playerNum, int32_t frameNum, uint32_t checksum);
	PacketType SendSystemMessage(uint8_t playerNum, std::string message);
	PacketType SendStartPos(uint8_t playerNum, uint8_t teamNum, uint8_t readyState, float x, float y, float z);
	PacketType SendPlayerInfo(uint8_t playerNum, float cpuUsage, int32_t ping);
	PacketType SendPlayerLeft(uint8_t playerNum, uint8_t bIntended);
	PacketType SendLogMsg(uint8_t playerNum, uint8_t logMsgLvl, const std::string& strData);
	PacketType SendLuaMsg(uint8_t playerNum, uint16_t script, uint8_t mode, const std::vector<uint8_t>& rawData);
	PacketType SendCurrentFrameProgress(int32_t frameNum);
	PacketType SendPing(uint8_t playerNum, uint8_t pingTag, float localTime);

	PacketType SendPlayerStat(uint8_t playerNum, const PlayerStatistics& currentStats);
	PacketType SendTeamStat(uint8_t teamNum, const TeamStatistics& currentStats);

	PacketType SendGiveAwayEverything(uint8_t playerNum, uint8_t giveToTeam);
	/**
	 * Gives everything from one team to another.
	 * The player issuing this command has to be leader of the takeFromTeam.
	 */
	PacketType SendGiveAwayEverything(uint8_t playerNum, uint8_t giveToTeam, uint8_t takeFromTeam);
	PacketType SendResign(uint8_t playerNum);
	PacketType SendJoinTeam(uint8_t playerNum, uint8_t wantedTeamNum);
	/**
	 * This is currently only used to inform the server about its death.
	 * It may have some problems when desync, because the team may not die
	 * on every client.
	 */
	PacketType SendTeamDied(uint8_t playerNum, uint8_t whichTeam);

	PacketType SendAICreated(uint8_t playerNum, uint8_t whichSkirmishAI, uint8_t team, const std::string& name);
	PacketType SendAIStateChanged(uint8_t playerNum, uint8_t whichSkirmishAI, uint8_t newState);

	PacketType SendSetAllied(uint8_t playerNum, uint8_t whichAllyTeam, uint8_t state);

	PacketType SendCreateNewPlayer( uint8_t playerNum, bool spectator, uint8_t teamNum, const std::string& playerName);

	PacketType SendClientData(uint8_t playerNum, const std::vector<uint8_t>& data);

#ifdef SYNCDEBUG
	PacketType SendSdCheckrequest(int32_t frameNum);
	PacketType SendSdCheckresponse(uint8_t playerNum, uint64_t flop, std::vector<uint32_t> checksums);
	PacketType SendSdReset();
	PacketType SendSdBlockrequest(uint16_t begin, uint16_t length, uint16_t requestSize);
	PacketType SendSdBlockresponse(uint8_t playerNum, std::vector<uint32_t> checksums);
#endif

private:
	CBaseNetProtocol();

};

#endif // _BASE_NET_PROTOCOL_H
