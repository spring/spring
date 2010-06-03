/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BASENETPROTOCOL_H
#define BASENETPROTOCOL_H

#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <vector>
#include <string>

namespace netcode
{
	class RawPacket;
}
struct PlayerStatistics;

const unsigned char NETWORK_VERSION = 1;

/*
Comment behind NETMSG enumeration constant gives the extra data belonging to
the net message. An empty comment means no extra data (message is only 1 byte).
Messages either consist of:
 1. uchar command; (NETMSG_* constant) and the specified extra data; or
 2. uchar command; uchar messageSize; and the specified extra data, for messages
    that contain a trailing std::string in the extra data; or
 3. uchar command; short messageSize; and the specified extra data, for messages
    that contain a trailing std::vector in the extra data.
Note that NETMSG_MAPDRAW can behave like 1. or 2. depending on the
CInMapDraw::NET_* command. messageSize is always the size of the entire message
including `command' and `messageSize'.
*/

enum NETMSG {
	NETMSG_KEYFRAME         = 1,  // int framenum
	NETMSG_NEWFRAME         = 2,  //
	NETMSG_QUIT             = 3,  // string reason
	NETMSG_STARTPLAYING     = 4,  // uint countdown
	NETMSG_SETPLAYERNUM     = 5,  // uchar myPlayerNum;
	NETMSG_PLAYERNAME       = 6,  // uchar myPlayerNum; std::string playerName;
	NETMSG_CHAT             = 7,  // uchar from, dest; std::string message;
	NETMSG_RANDSEED         = 8,  // uint randSeed;
	NETMSG_GAMEID           = 9,  // uchar gameID[16];
	NETMSG_PATH_CHECKSUM    = 10, // uchar myPlayerNum, boost::uint32_t checksum
	NETMSG_COMMAND          = 11, // uchar myPlayerNum; int id; uchar options; std::vector<float> params;
	NETMSG_SELECT           = 12, // uchar myPlayerNum; std::vector<short> selectedUnitIDs;
	NETMSG_PAUSE            = 13, // uchar playerNum, bPaused;

	NETMSG_AICOMMAND        = 14, // uchar myPlayerNum; short unitID; int id; uchar options; std::vector<float> params;
	NETMSG_AICOMMANDS       = 15, // uchar myPlayerNum;
	                              // short unitIDCount;  unitIDCount X short(unitID)
	                              // short commandCount; commandCount X { int id; uchar options; std::vector<float> params }
	NETMSG_AISHARE          = 16, // uchar myPlayerNum, uchar sourceTeam, uchar destTeam, float metal, float energy, std::vector<short> unitIDs

	NETMSG_MEMDUMP          = 17, // (NEVER SENT)
	NETMSG_USER_SPEED       = 19, // uchar myPlayerNum, float userSpeed;
	NETMSG_INTERNAL_SPEED   = 20, // float internalSpeed;
	NETMSG_CPU_USAGE        = 21, // float cpuUsage;
	NETMSG_DIRECT_CONTROL   = 22, // uchar myPlayerNum;
	NETMSG_DC_UPDATE        = 23, // uchar myPlayerNum, status; short heading, pitch;
	NETMSG_ATTEMPTCONNECT   = 25, // ushort msgsize, string playername, string passwd, string VERSION_STRING_DETAILED
	NETMSG_SHARE            = 26, // uchar myPlayerNum, shareTeam, bShareUnits; float shareMetal, shareEnergy;
	NETMSG_SETSHARE         = 27, // uchar myPlayerNum, uchar myTeam; float metalShareFraction, energyShareFraction;
	NETMSG_SENDPLAYERSTAT   = 28, //
	NETMSG_PLAYERSTAT       = 29, // uchar myPlayerNum; CPlayer::Statistics currentStats;
	NETMSG_GAMEOVER         = 30, //
	NETMSG_MAPDRAW          = 31, // uchar messageSize =  8, myPlayerNum, command = CInMapDraw::NET_ERASE; short x, z;
	                              // uchar messageSize = 12, myPlayerNum, command = CInMapDraw::NET_LINE; short x1, z1, x2, z2;
	                              // /*messageSize*/   uchar myPlayerNum, command = CInMapDraw::NET_POINT; short x, z; std::string label;
	NETMSG_SYNCRESPONSE     = 33, // uchar myPlayerNum; int frameNum; uint checksum;
	NETMSG_SYSTEMMSG        = 35, // uchar myPlayerNum, std::string message;
	NETMSG_STARTPOS         = 36, // uchar myPlayerNum, uchar myTeam, ready /*0: not ready, 1: ready, 2: don't update readiness*/; float x, y, z;
	NETMSG_PLAYERINFO       = 38, // uchar myPlayerNum; float cpuUsage; int ping /*in frames*/;
	NETMSG_PLAYERLEFT       = 39, // uchar myPlayerNum, bIntended /*0: lost connection, 1: left, 2: forced (kicked) */;

#ifdef SYNCDEBUG
	NETMSG_SD_CHKREQUEST    = 41,
	NETMSG_SD_CHKRESPONSE   = 42,
	NETMSG_SD_BLKREQUEST    = 43,
	NETMSG_SD_BLKRESPONSE   = 44,
	NETMSG_SD_RESET         = 45,
#endif // SYNCDEBUG

	NETMSG_LUAMSG           = 50, // /* uint16_t messageSize */, uchar myPlayerNum, unsigned short script, uchar mode, std::vector<uint8_t> msg
	NETMSG_TEAM             = 51, // uchar myPlayerNum, uchar action, uchar parameter1
	NETMSG_GAMEDATA         = 52, // /* uchar messageSize */, std::string setupText, std::string script, std::string map, int mapChecksum,
	                              // std::string mod, int modChecksum, int randomSeed (each string ends with \0)
	NETMSG_ALLIANCE         = 53, // uchar myPlayerNum, uchar otherAllyTeam, uchar allianceState (0 = not allied / 1 = allied)
	NETMSG_CCOMMAND         = 54, // /* short! messageSize */, int! myPlayerNum, std::string command, std::string extra (each string ends with \0)
	NETMSG_CUSTOM_DATA      = 55, // uchar myPlayerNum, uchar dataType, uchar dataValue

	NETMSG_AI_CREATED       = 70, // /* uchar messageSize */, uchar myPlayerNum, uint whichSkirmishAI, uchar team, std::string name (ends with \0)
	NETMSG_AI_STATE_CHANGED = 71, // uchar myPlayerNum, uint whichSkirmishAI, uchar newState

	NETMSG_REGISTER_NETMSG	= 73, // uchar myPlayerNum, uchar NETMSG
	NETMSG_UNREGISTER_NETMSG= 74  // uchar myPlayerNum, uchar NETMSG
};

// Data types for NETMSG_CUSTOM_DATA
#define CUSTOM_DATA_SPEEDCONTROL 0

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
@brief A factory used to make often-used network messages.

Use this if you want to create a network message. Implemented as a singleton.
 */
class CBaseNetProtocol
{
public:
	typedef unsigned char uchar;
	typedef unsigned int uint;
	typedef boost::shared_ptr<const netcode::RawPacket> PacketType;

	static CBaseNetProtocol& Get();

	PacketType SendKeyFrame(int frameNum);
	PacketType SendNewFrame();
	PacketType SendQuit(const std::string& reason);
	PacketType SendStartPlaying(unsigned countdown); ///< client can send these to force-start the game
	PacketType SendSetPlayerNum(uchar myPlayerNum);
	PacketType SendPlayerName(uchar myPlayerNum, const std::string& playerName);
	PacketType SendRandSeed(uint randSeed);
	PacketType SendGameID(const uchar* buf);
	PacketType SendPathCheckSum(uchar myPlayerNum, boost::uint32_t checksum);
	PacketType SendCommand(uchar myPlayerNum, int id, uchar options, const std::vector<float>& params);
	PacketType SendSelect(uchar myPlayerNum, const std::vector<short>& selectedUnitIDs);
	PacketType SendPause(uchar myPlayerNum, uchar bPaused);

	PacketType SendAICommand(uchar myPlayerNum, short unitID, int id, uchar options, const std::vector<float>& params);
	PacketType SendAIShare(uchar myPlayerNum, uchar sourceTeam, uchar destTeam, float metal, float energy, const std::vector<short>& unitIDs);

	PacketType SendUserSpeed(uchar myPlayerNum, float userSpeed);
	PacketType SendInternalSpeed(float internalSpeed);
	PacketType SendCPUUsage(float cpuUsage);
	PacketType SendCustomData(uchar myPlayerNum, uchar dataType, int dataValue);
	PacketType SendSpeedControl(uchar myPlayerNum, int speedCtrl);
	PacketType SendDirectControl(uchar myPlayerNum);
	PacketType SendDirectControlUpdate(uchar myPlayerNum, uchar status, short heading, short pitch);
	PacketType SendAttemptConnect(const std::string name, const std::string& passwd, const std::string version, bool reconnect = false);
	PacketType SendShare(uchar myPlayerNum, uchar shareTeam, uchar bShareUnits, float shareMetal, float shareEnergy);
	PacketType SendSetShare(uchar myPlayerNum, uchar myTeam, float metalShareFraction, float energyShareFraction);
	PacketType SendSendPlayerStat();
	PacketType SendPlayerStat(uchar myPlayerNum, const PlayerStatistics& currentStats);
	PacketType SendGameOver();
	PacketType SendMapErase(uchar myPlayerNum, short x, short z);
	PacketType SendMapDrawLine(uchar myPlayerNum, short x1, short z1, short x2, short z2, bool);
	PacketType SendMapDrawPoint(uchar myPlayerNum, short x, short z, const std::string& label, bool);
	PacketType SendSyncResponse(int frameNum, uint checksum);
	PacketType SendSystemMessage(uchar myPlayerNum, std::string message);
	PacketType SendStartPos(uchar myPlayerNum, uchar teamNum, uchar ready, float x, float y, float z);
	PacketType SendPlayerInfo(uchar myPlayerNum, float cpuUsage, int ping);
	PacketType SendPlayerLeft(uchar myPlayerNum, uchar bIntended);
	PacketType SendLuaMsg(uchar myPlayerNum, unsigned short script, uchar mode, const std::vector<boost::uint8_t>& msg);

	PacketType SendGiveAwayEverything(uchar myPlayerNum, uchar giveToTeam);
	/**
	 * Gives everything from one team to an other.
	 * The player issuing this command has to be leader of the takeFromTeam.
	 */
	PacketType SendGiveAwayEverything(uchar myPlayerNum, uchar giveToTeam, uchar takeFromTeam);
	PacketType SendResign(uchar myPlayerNum);
	PacketType SendJoinTeam(uchar myPlayerNum, uchar wantedTeamNum);
	/**
	 * This is currently only used to inform the server about its death.
	 * It may have some problems when desync, because the team may not die
	 * on every client.
	 */
	PacketType SendTeamDied(uchar myPlayerNum, uchar whichTeam);

	PacketType SendAICreated(const uchar myPlayerNum,
	                         const uint  whichSkirmishAI,
	                         const uchar team,
	                         const std::string& name);
	PacketType SendAIStateChanged(const uchar myPlayerNum,
	                              const uint  whichSkirmishAI,
	                              const uchar newState);

	PacketType SendSetAllied(uchar myPlayerNum, uchar whichAllyTeam, uchar state);

	PacketType SendRegisterNetMsg( uchar myPlayerNum, NETMSG msgID );
	PacketType SendUnRegisterNetMsg( uchar myPlayerNum, NETMSG msgID );

#ifdef SYNCDEBUG
	PacketType SendSdCheckrequest(int frameNum);
	PacketType SendSdCheckresponse(uchar myPlayerNum, boost::uint64_t flop, std::vector<unsigned> checksums);
	PacketType SendSdReset();
	PacketType SendSdBlockrequest(unsigned short begin, unsigned short length, unsigned short requestSize);
	PacketType SendSdBlockresponse(uchar myPlayerNum, std::vector<unsigned> checksums);
#endif
private:
	CBaseNetProtocol();
	~CBaseNetProtocol();
};

#endif // BASENETPROTOCOL_H
