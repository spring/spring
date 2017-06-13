/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASE_NET_PROTOCOL_H
#define _BASE_NET_PROTOCOL_H

#include <memory>
#include <cinttypes>
#include <vector>
#include <string>
#include <stdlib.h>
#include "Game/GameVersion.h"


namespace netcode
{
	class RawPacket;
}
struct PlayerStatistics;
struct TeamStatistics;


static const uint16_t NETWORK_VERSION = atoi(SpringVersion::GetMajor().c_str());


/*
 * Comment behind NETMSG enumeration constant gives the extra data belonging to
 * the net message.
 * An empty comment means no extra data (message is only 1 byte).
 * Messages either consist of:
 *  1. uint8_t command; (NETMSG_* constant) and the specified extra data; or
 *  2. uint8_t command; uint8_t messageSize; and the specified extra data,
 *     for messages that contain a trailing std::string in the extra data; or
 *  3. uint8_t command; uint16_t messageSize; and the specified extra data,
 *     for messages that contain a trailing std::vector in the extra data.
 * Note that NETMSG_MAPDRAW can behave like 1. or 2. depending on the
 * MapDrawAction::NET_* command. messageSize is always the size of the entire
 * message in bytes, including 'command' and 'messageSize'.
 */

enum NETMSG {
	NETMSG_KEYFRAME         = 1,  // int32_t framenum
	NETMSG_NEWFRAME         = 2,  //
	NETMSG_QUIT             = 3,  // string reason
	NETMSG_STARTPLAYING     = 4,  // uint32_t countdown
	NETMSG_SETPLAYERNUM     = 5,  // uint8_t myPlayerNum;
	NETMSG_PLAYERNAME       = 6,  // uint8_t myPlayerNum; std::string playerName;
	NETMSG_CHAT             = 7,  // uint8_t from, dest; std::string message;
	NETMSG_RANDSEED         = 8,  // uint32_t randSeed;
	NETMSG_GAMEID           = 9,  // uint8_t gameID[16];
	NETMSG_PATH_CHECKSUM    = 10, // uint8_t myPlayerNum, uint32_t checksum
	NETMSG_COMMAND          = 11, // uint8_t myPlayerNum; int32_t id; uint8_t options; std::vector<float> params;
	NETMSG_SELECT           = 12, // uint8_t myPlayerNum; std::vector<int16_t> selectedUnitIDs;
	NETMSG_PAUSE            = 13, // uint8_t playerNum, bPaused;

	NETMSG_AICOMMAND        = 14, // uint8_t myPlayerNum; uint8_t aiID; int16_t unitID; int32_t id; uint8_t options; std::vector<float> params;
	NETMSG_AICOMMANDS       = 15, // uint8_t myPlayerNum; uint8_t aiID; uint8_t pairwise, uint32_t sameCmdID, uint8_t sameCmdOpt, uint16_t sameCmdParamSize;
	                              // int16_t unitIDCount;  unitIDCount * int16_t(unitID)
	                              // int16_t commandCount; commandCount * { int32_t id; uint8_t options; std::vector<float> params }
	NETMSG_AISHARE          = 16, // uint8_t myPlayerNum, uint8_t aiID, uint8_t sourceTeam, uint8_t destTeam, float metal, float energy, std::vector<int16_t> unitIDs

	NETMSG_USER_SPEED       = 19, // uint8_t myPlayerNum, float userSpeed;
	NETMSG_INTERNAL_SPEED   = 20, // float internalSpeed;
	NETMSG_CPU_USAGE        = 21, // float cpuUsage;
	NETMSG_DIRECT_CONTROL   = 22, // uint8_t myPlayerNum;
	NETMSG_DC_UPDATE        = 23, // uint8_t myPlayerNum, status; int16_t heading, pitch;
	NETMSG_SHARE            = 26, // uint8_t myPlayerNum, shareTeam, bShareUnits; float shareMetal, shareEnergy;
	NETMSG_SETSHARE         = 27, // uint8_t myPlayerNum, uint8_t myTeam; float metalShareFraction, energyShareFraction;

	NETMSG_PLAYERSTAT       = 29, // uint8_t myPlayerNum; CPlayer::Statistics currentStats;
	NETMSG_GAMEOVER         = 30, // uint8_t myPlayerNum; std::vector<uint8_t> winningAllyTeams
	NETMSG_MAPDRAW          = 31, // uint8_t messageSize =  8, myPlayerNum, command = MapDrawAction::NET_ERASE; int16_t x, z;
	                              // uint8_t messageSize = 12, myPlayerNum, command = MapDrawAction::NET_LINE; int16_t x1, z1, x2, z2;
	                              // /*messageSize*/   uint8_t myPlayerNum, command = MapDrawAction::NET_POINT; int16_t x, z; std::string label;
	NETMSG_SYNCRESPONSE     = 33, // uint8_t myPlayerNum; int32_t frameNum; uint32_t checksum;
	NETMSG_SYSTEMMSG        = 35, // uint8_t myPlayerNum, std::string message;
	NETMSG_STARTPOS         = 36, // uint8_t myPlayerNum, uint8_t myTeam, ready /*0: not ready, 1: ready, 2: don't update readiness*/; float x, y, z;
	NETMSG_PLAYERINFO       = 38, // uint8_t myPlayerNum; float cpuUsage; int32_t ping /*in milliseconds*/;
	NETMSG_PLAYERLEFT       = 39, // uint8_t myPlayerNum, bIntended /*0: lost connection, 1: left, 2: forced (kicked) */;

#ifdef SYNCDEBUG
	NETMSG_SD_CHKREQUEST    = 41,
	NETMSG_SD_CHKRESPONSE   = 42,
	NETMSG_SD_BLKREQUEST    = 43,
	NETMSG_SD_BLKRESPONSE   = 44,
	NETMSG_SD_RESET         = 45,
#endif // SYNCDEBUG

	NETMSG_LOGMSG           = 49, // uint8_t myPlayerNum, std::string message
	NETMSG_LUAMSG           = 50, // /* uint16_t messageSize */, uint8_t myPlayerNum, uint16_t script, uint8_t mode, std::vector<uint8_t> msg

	NETMSG_TEAM             = 51, // uint8_t myPlayerNum, uint8_t action, uint8_t parameter1
	NETMSG_GAMEDATA         = 52, // /* uint8_t messageSize */, std::string setupText, std::string script, std::string map, int32_t mapChecksum,
	                              // std::string mod, int32_t modChecksum, int32_t randomSeed (each string ends with \0)
	NETMSG_ALLIANCE         = 53, // uint8_t myPlayerNum, uint8_t otherAllyTeam, uint8_t allianceState (0 = not allied / 1 = allied)
	NETMSG_CCOMMAND         = 54, // /* int16_t! messageSize */, int! myPlayerNum, std::string command, std::string extra (each string ends with \0)
	NETMSG_TEAMSTAT         = 60, // uint8_t teamNum, struct TeamStatistics statistics      # used by LadderBot #
	NETMSG_CLIENTDATA       = 61, // uint16_t messageSize, std::string setupText

	NETMSG_ATTEMPTCONNECT   = 65, // uint16_t msgsize, uint16_t netversion, string playername, string passwd, string VERSION_STRING_DETAILED
	NETMSG_REJECT_CONNECT   = 66, // string reason

	NETMSG_AI_CREATED       = 70, // /* uint8_t messageSize */, uint8_t myPlayerNum, uint8_t whichSkirmishAI, uint8_t team, std::string name (ends with \0)
	NETMSG_AI_STATE_CHANGED = 71, // uint8_t myPlayerNum, uint8_t whichSkirmishAI, uint8_t newState

	NETMSG_REQUEST_TEAMSTAT = 72, // uint8_t teamNum, uint16_t statFrameNum                   # used by LadderBot #

	NETMSG_CREATE_NEWPLAYER = 75, // uint8_t myPlayerNum, uint8_t spectator, uint8_t teamNum, std::string playerName #used for players not preset in script.txt#

	NETMSG_AICOMMAND_TRACKED= 76,  // uint8_t myPlayerNum; uint8_t aiID; int16_t unitID; int32_t id; uint8_t options; int32_t aiCommandId, std::vector<float> params;

	NETMSG_GAME_FRAME_PROGRESS= 77, // int32_t frameNum # this special packet skips queue & cache entirely, indicates current game progress for clients fast-forwarding to current point the game #


	NETMSG_LAST //max types of netmessages, internal only
};


/// sub-action-types of NETMSG_TEAM
enum TEAMMSG {
//	TEAMMSG_NAME            = number    parameter1, ...
	TEAMMSG_GIVEAWAY        = 1,     // team to give stuff to, team to take stuff from (player has to be leader of the team)
	TEAMMSG_RESIGN          = 2,     // not used
	TEAMMSG_JOIN_TEAM       = 3,     // team to join
	TEAMMSG_TEAM_DIED       = 4,     // team which had died special note: this is sent by all players to prevent cheating
//TODO: changing teams (to spectator, from spectator to specific team)
//TODO: in-game allyteams
};

/// sub-action-types of NETMSG_MAPDRAW
enum MapDrawAction {
	MAPDRAW_POINT,
	MAPDRAW_ERASE,
	MAPDRAW_LINE
};

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
	PacketType SendSetPlayerNum(uint8_t myPlayerNum);
	PacketType SendPlayerName(uint8_t myPlayerNum, const std::string& playerName);
	PacketType SendRandSeed(uint32_t randSeed);
	PacketType SendGameID(const uint8_t* buf);
	PacketType SendPathCheckSum(uint8_t myPlayerNum, uint32_t checksum);
	PacketType SendCommand(uint8_t myPlayerNum, int32_t id, uint8_t options, const std::vector<float>& params);
	PacketType SendSelect(uint8_t myPlayerNum, const std::vector<int16_t>& selectedUnitIDs);
	PacketType SendPause(uint8_t myPlayerNum, uint8_t bPaused);

	PacketType SendAICommand(uint8_t myPlayerNum, uint8_t aiID, int16_t unitID, int32_t commandID, int32_t aiCommandID, uint8_t options, const std::vector<float>& params);
	PacketType SendAIShare(uint8_t myPlayerNum, uint8_t aiID, uint8_t sourceTeam, uint8_t destTeam, float metal, float energy, const std::vector<int16_t>& unitIDs);

	PacketType SendUserSpeed(uint8_t myPlayerNum, float userSpeed);
	PacketType SendInternalSpeed(float internalSpeed);
	PacketType SendCPUUsage(float cpuUsage);
	PacketType SendCustomData(uint8_t myPlayerNum, uint8_t dataType, int32_t dataValue);
	PacketType SendLuaDrawTime(uint8_t myPlayerNum, int32_t mSec);
	PacketType SendDirectControl(uint8_t myPlayerNum);
	PacketType SendDirectControlUpdate(uint8_t myPlayerNum, uint8_t status, int16_t heading, int16_t pitch);
	PacketType SendAttemptConnect(const std::string& name, const std::string& passwd, const std::string& version, int32_t netloss, bool reconnect = false);
	PacketType SendRejectConnect(const std::string& reason);
	PacketType SendShare(uint8_t myPlayerNum, uint8_t shareTeam, uint8_t bShareUnits, float shareMetal, float shareEnergy);
	PacketType SendSetShare(uint8_t myPlayerNum, uint8_t myTeam, float metalShareFraction, float energyShareFraction);
	PacketType SendGameOver(uint8_t myPlayerNum, const std::vector<uint8_t>& winningAllyTeams);
	PacketType SendMapErase(uint8_t myPlayerNum, int16_t x, int16_t z);
	PacketType SendMapDrawLine(uint8_t myPlayerNum, int16_t x1, int16_t z1, int16_t x2, int16_t z2, bool);
	PacketType SendMapDrawPoint(uint8_t myPlayerNum, int16_t x, int16_t z, const std::string& label, bool);
	PacketType SendSyncResponse(uint8_t myPlayerNum, int32_t frameNum, uint32_t checksum);
	PacketType SendSystemMessage(uint8_t myPlayerNum, std::string message);
	PacketType SendStartPos(uint8_t myPlayerNum, uint8_t teamNum, uint8_t readyState, float x, float y, float z);
	PacketType SendPlayerInfo(uint8_t myPlayerNum, float cpuUsage, int32_t ping);
	PacketType SendPlayerLeft(uint8_t myPlayerNum, uint8_t bIntended);
	PacketType SendLogMsg(uint8_t myPlayerNum, const std::string& msg);
	PacketType SendLuaMsg(uint8_t myPlayerNum, uint16_t script, uint8_t mode, const std::vector<uint8_t>& msg);
	PacketType SendCurrentFrameProgress(int32_t frameNum);

	PacketType SendPlayerStat(uint8_t myPlayerNum, const PlayerStatistics& currentStats);
	PacketType SendTeamStat(uint8_t teamNum, const TeamStatistics& currentStats);

	PacketType SendGiveAwayEverything(uint8_t myPlayerNum, uint8_t giveToTeam);
	/**
	 * Gives everything from one team to another.
	 * The player issuing this command has to be leader of the takeFromTeam.
	 */
	PacketType SendGiveAwayEverything(uint8_t myPlayerNum, uint8_t giveToTeam, uint8_t takeFromTeam);
	PacketType SendResign(uint8_t myPlayerNum);
	PacketType SendJoinTeam(uint8_t myPlayerNum, uint8_t wantedTeamNum);
	/**
	 * This is currently only used to inform the server about its death.
	 * It may have some problems when desync, because the team may not die
	 * on every client.
	 */
	PacketType SendTeamDied(uint8_t myPlayerNum, uint8_t whichTeam);

	PacketType SendAICreated(uint8_t myPlayerNum, uint8_t whichSkirmishAI, uint8_t team, const std::string& name);
	PacketType SendAIStateChanged(uint8_t myPlayerNum, uint8_t whichSkirmishAI, uint8_t newState);

	PacketType SendSetAllied(uint8_t myPlayerNum, uint8_t whichAllyTeam, uint8_t state);

	PacketType SendCreateNewPlayer( uint8_t playerNum, bool spectator, uint8_t teamNum, std::string playerName);

	PacketType SendClientData(uint8_t playerNum, const std::vector<uint8_t>& data);

#ifdef SYNCDEBUG
	PacketType SendSdCheckrequest(int32_t frameNum);
	PacketType SendSdCheckresponse(uint8_t myPlayerNum, uint64_t flop, std::vector<uint32_t> checksums);
	PacketType SendSdReset();
	PacketType SendSdBlockrequest(uint16_t begin, uint16_t length, uint16_t requestSize);
	PacketType SendSdBlockresponse(uint8_t myPlayerNum, std::vector<uint32_t> checksums);
#endif

private:
	CBaseNetProtocol();

};

#endif // _BASE_NET_PROTOCOL_H
