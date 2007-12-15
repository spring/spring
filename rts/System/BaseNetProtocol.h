#ifndef BASENETPROTOCOL_H
#define BASENETPROTOCOL_H

#include "Net/Net.h"
#include "Game/Player.h"


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
	NETMSG_QUIT             = 2,  //
	NETMSG_NEWFRAME         = 3,  // int frameNum;
	NETMSG_STARTPLAYING     = 4,  //
	NETMSG_SETPLAYERNUM     = 5,  // uchar myPlayerNum;
	NETMSG_PLAYERNAME       = 6,  // uchar myPlayerNum; std::string playerName;
	NETMSG_CHAT             = 7,  // uchar myPlayerNum; std::string message;
	NETMSG_RANDSEED         = 8,  // uint randSeed;
	NETMSG_GAMEID           = 9,  // uchar gameID[16];
	NETMSG_COMMAND          = 11, // uchar myPlayerNum; int id; uchar options; std::vector<float> params;
	NETMSG_SELECT           = 12, // uchar myPlayerNum; std::vector<short> selectedUnitIDs;
	NETMSG_PAUSE            = 13, // uchar playerNum, bPaused;
	NETMSG_AICOMMAND        = 14, // uchar myPlayerNum; short unitID; int id; uchar options; std::vector<float> params;
	NETMSG_AICOMMANDS       = 15, // uchar myPlayerNum;
	                              // short unitIDCount;  unitIDCount X short(unitID)
	                              // short commandCount; commandCount X { int id; uchar options; std::vector<float> params }
	NETMSG_SCRIPT           = 16, // std::string scriptName;
	NETMSG_MEMDUMP          = 17, // (NEVER SENT)
	NETMSG_MAPNAME          = 18, // uint checksum; std::string mapName;   (e.g. `SmallDivide.smf')
	NETMSG_USER_SPEED       = 19, // uchar myPlayerNum, float userSpeed;
	NETMSG_INTERNAL_SPEED   = 20, // float internalSpeed;
	NETMSG_CPU_USAGE        = 21, // float cpuUsage;
	NETMSG_DIRECT_CONTROL   = 22, // uchar myPlayerNum;
	NETMSG_DC_UPDATE        = 23, // uchar myPlayerNum, status; short heading, pitch;
	NETMSG_ATTEMPTCONNECT   = 25, // uchar myPlayerNum, networkVersion;
	NETMSG_SHARE            = 26, // uchar myPlayerNum, shareTeam, bShareUnits; float shareMetal, shareEnergy;
	NETMSG_SETSHARE         = 27, // uchar myPlayerNum, uchar myTeam; float metalShareFraction, energyShareFraction;
	NETMSG_SENDPLAYERSTAT   = 28, //
	NETMSG_PLAYERSTAT       = 29, // uchar myPlayerNum; CPlayer::Statistics currentStats;
	NETMSG_GAMEOVER         = 30, //
	NETMSG_MAPDRAW          = 31, // uchar messageSize =  8, myPlayerNum, command = CInMapDraw::NET_ERASE; short x, z;
	                              // uchar messageSize = 12, myPlayerNum, command = CInMapDraw::NET_LINE; short x1, z1, x2, z2;
	                              // /*messageSize*/   uchar myPlayerNum, command = CInMapDraw::NET_POINT; short x, z; std::string label;
	NETMSG_SYNCREQUEST      = 32, // int frameNum;
	NETMSG_SYNCRESPONSE     = 33, // uchar myPlayerNum; int frameNum; uint checksum;
	NETMSG_SYSTEMMSG        = 35, // uchar myPlayerNum, std::string message;
	NETMSG_STARTPOS         = 36, // uchar myPlayerNum, uchar myTeam, ready /*0: not ready, 1: ready, 2: don't update readiness*/; float x, y, z;
	NETMSG_PLAYERINFO       = 38, // uchar myPlayerNum; float cpuUsage; int ping /*in frames*/;
	NETMSG_PLAYERLEFT       = 39, // uchar myPlayerNum, bIntended /*0: lost connection, 1: left, 2: forced (kicked) */;
	NETMSG_MODNAME          = 40, // uint checksum; std::string modName;   (e.g. `XTA v8.1')
#ifdef SYNCDEBUG
	NETMSG_SD_CHKREQUEST    = 41,
	NETMSG_SD_CHKRESPONSE   = 42,
	NETMSG_SD_BLKREQUEST    = 43,
	NETMSG_SD_BLKRESPONSE   = 44,
	NETMSG_SD_RESET         = 45,
#endif // SYNCDEBUG
	NETMSG_LUAMSG           = 50, // uchar myPlayerNum, std::string msg
};

/**
@brief High level network code layer
Provides protocoldependent functions over our CNet-Class. It includes all functions needed to send stuff without handling with the internals.
 */
class CBaseNetProtocol : public netcode::CNet {
public:
	typedef unsigned char uchar;
	typedef unsigned int uint;

	CBaseNetProtocol();
	~CBaseNetProtocol();

	/**
	@brief  Broadcast raw data to all clients
	Should not be used. Use Send*(...) instead, only redirects to CNet::SendData(char*, int);
	@todo make everything use the Send* functions
	*/
	void RawSend(const uchar* data,const unsigned length);

	void SendQuit();
	void SendQuit(unsigned playerNum);
	void SendNewFrame(int frameNum);
	void SendStartPlaying();
	void SendSetPlayerNum(uchar myPlayerNum, uchar connNumber);
	void SendPlayerName(uchar myPlayerNum, const std::string& playerName);
	void SendChat(uchar myPlayerNum, const std::string& message);
	void SendRandSeed(uint randSeed);
	void SendGameID(const uchar* buf);
	void SendCommand(uchar myPlayerNum, int id, uchar options, const std::vector<float>& params);
	void SendSelect(uchar myPlayerNum, const std::vector<short>& selectedUnitIDs);
	void SendPause(uchar myPlayerNum, uchar bPaused);
	void SendAICommand(uchar myPlayerNum, short unitID, int id, uchar options, const std::vector<float>& params);
	void SendAICommands(uchar myPlayerNum, short unitIDCount, ...);
	void SendScript(const std::string& scriptName);
	void SendMapName(const uint checksum, const std::string& mapName);
	void SendUserSpeed(uchar myPlayerNum, float userSpeed);
	void SendInternalSpeed(float internalSpeed);
	void SendCPUUsage(float cpuUsage);
	void SendDirectControl(uchar myPlayerNum);
	void SendDirectControlUpdate(uchar myPlayerNum, uchar status, short heading, short pitch);
	void SendAttemptConnect(uchar myPlayerNum, uchar networkVersion);
	void SendShare(uchar myPlayerNum, uchar shareTeam, uchar bShareUnits, float shareMetal, float shareEnergy);
	void SendSetShare(uchar myPlayerNum, uchar myTeam, float metalShareFraction, float energyShareFraction);
	void SendSendPlayerStat();
	void SendPlayerStat(uchar myPlayerNum, const CPlayer::Statistics& currentStats);
	void SendGameOver();
	void SendMapErase(uchar myPlayerNum, short x, short z);
	void SendMapDrawLine(uchar myPlayerNum, short x1, short z1, short x2, short z2);
	void SendMapDrawPoint(uchar myPlayerNum, short x, short z, const std::string& label);
	void SendSyncRequest(int frameNum);
	void SendSyncResponse(uchar myPlayerNum, int frameNum, uint checksum);
	void SendSystemMessage(uchar myPlayerNum, const std::string& message);
	void SendStartPos(uchar myPlayerNum, uchar teamNum, uchar ready, float x, float y, float z);
	void SendPlayerInfo(uchar myPlayerNum, float cpuUsage, int ping);
	void SendPlayerLeft(uchar myPlayerNum, uchar bIntended);
	void SendModName(const uint checksum, const std::string& modName);
	void SendLuaMsg(uchar myPlayerNum, uchar script, uchar mode, const std::string& msg);

private:
};

#endif

