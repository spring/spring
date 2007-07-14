#ifndef NETPROTOCOL_H
#define NETPROTOCOL_H

#include "Net.h"
#include "Game/Player.h"
#include "Demo.h"


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
	// WARNING: This is now in CNet, but dont use thos number for anything else
	// tvo: CGame and CPreGame still need to throw it away...
	NETMSG_HELLO            = 1,  // reserved for CNet
	NETMSG_QUIT             = 2,  //
	NETMSG_NEWFRAME         = 3,  // int frameNum;
	NETMSG_STARTPLAYING     = 4,  //
	NETMSG_SETPLAYERNUM     = 5,  // uchar myPlayerNum;
	NETMSG_PLAYERNAME       = 6,  // uchar myPlayerNum; std::string playerName;
	NETMSG_CHAT             = 7,  // uchar myPlayerNum; std::string message;
	NETMSG_RANDSEED         = 8,  // uint randSeed;
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
	NETMSG_USER_SPEED       = 19, // float userSpeed;
	NETMSG_INTERNAL_SPEED   = 20, // float internalSpeed;
	NETMSG_CPU_USAGE        = 21, // float cpuUsage;
	NETMSG_DIRECT_CONTROL   = 22, // uchar myPlayerNum;
	NETMSG_DC_UPDATE        = 23, // uchar myPlayerNum, status; short heading, pitch;
	NETMSG_ATTEMPTCONNECT   = 25, // uchar myPlayerNum, networkVersion;
	NETMSG_SHARE            = 26, // uchar myPlayerNum, shareTeam, bShareUnits; float shareMetal, shareEnergy;
	NETMSG_SETSHARE         = 27, // uchar myTeam; float metalShareFraction, energyShareFraction;
	NETMSG_SENDPLAYERSTAT   = 28, //
	NETMSG_PLAYERSTAT       = 29, // uchar myPlayerNum; CPlayer::Statistics currentStats;
	NETMSG_GAMEOVER         = 30, //
	NETMSG_MAPDRAW          = 31, // uchar messageSize =  8, myPlayerNum, command = CInMapDraw::NET_ERASE; short x, z;
	                              // uchar messageSize = 12, myPlayerNum, command = CInMapDraw::NET_LINE; short x1, z1, x2, z2;
	                              // /*messageSize*/   uchar myPlayerNum, command = CInMapDraw::NET_POINT; short x, z; std::string label;
	NETMSG_SYNCREQUEST      = 32, // int frameNum;
	NETMSG_SYNCRESPONSE     = 33, // uchar myPlayerNum; int frameNum; uint checksum;
	NETMSG_SYSTEMMSG        = 35, // uchar myPlayerNum; std::string message;
	NETMSG_STARTPOS         = 36, // uchar myTeam, ready /*0: not ready, 1: ready, 2: don't update readiness*/; float x, y, z;
	NETMSG_PLAYERINFO       = 38, // uchar myPlayerNum; float cpuUsage; int ping /*in frames*/;
	NETMSG_PLAYERLEFT       = 39, // uchar myPlayerNum, bIntended /*0: lost connection, 1: left*/;
	NETMSG_MODNAME          = 40, // uint checksum; std::string modName;   (e.g. `XTA v8.1')
#ifdef SYNCDEBUG
	NETMSG_SD_CHKREQUEST    = 41,
	NETMSG_SD_CHKRESPONSE   = 42,
	NETMSG_SD_BLKREQUEST    = 43,
	NETMSG_SD_BLKRESPONSE   = 44,
	NETMSG_SD_RESET         = 45,
#endif // SYNCDEBUG
};

/**
@brief High level network code layer
@TODO drop Send-commands if we are client from a demo host
*/
class CNetProtocol : public netcode::CNet {
public:
	typedef unsigned char uchar;
	typedef unsigned int uint;

	CNetProtocol();
	~CNetProtocol();

	int InitServer(const unsigned portnum);
	/**
	@brief use this if you want to host a demo file
	This is the best way to see a demo, this should also be used when only one local client will watch this
	*/
	int InitServer(const unsigned portnum, const std::string& demoName);
	int InitClient(const char* server,unsigned portnum,unsigned sourceport);
	/// Initialise our client to listen to CLocalConnection
	int InitLocalClient(const unsigned wantedNumber);
	/// This will tell our server that we have a CLocalConnection
	int ServerInitLocalClient(const unsigned wantedNumber);

	/// Check for new incoming data / connections
	void Update();

	bool IsDemoServer() const;
	bool localDemoPlayback;

	/**
	@brief  Broadcast raw data to all clients
	Should not be used. Use Send*(...) instead, only redirects to CNet::SendData(char*, int);
	@TODO make everything use the Send* functions
	 */
	void RawSend(const uchar* data,const unsigned length);

	int GetData(unsigned char* buf,const unsigned length, const unsigned conNum, int* que = NULL);

	void SendHello();
	int SendQuit();
	int SendNewFrame(int frameNum);
	int SendStartPlaying();
	int SendSetPlayerNum(uchar myPlayerNum);
	int SendPlayerName(uchar myPlayerNum, const std::string& playerName);
	int SendChat(uchar myPlayerNum, const std::string& message);
	int SendRandSeed(uint randSeed);
	int SendCommand(uchar myPlayerNum, int id, uchar options, const std::vector<float>& params);
	int SendSelect(uchar myPlayerNum, const std::vector<short>& selectedUnitIDs);
	int SendPause(uchar myPlayerNum, uchar bPaused);
	int SendAICommand(uchar myPlayerNum, short unitID, int id, uchar options, const std::vector<float>& params);
	int SendAICommands(uchar myPlayerNum, short unitIDCount, ...);
	int SendScript(const std::string& scriptName);
	int SendMapName(const uint checksum, const std::string& mapName);
	int SendUserSpeed(float userSpeed);
	int SendInternalSpeed(float internalSpeed);
	int SendCPUUsage(float cpuUsage);
	int SendDirectControl(uchar myPlayerNum);
	int SendDirectControlUpdate(uchar myPlayerNum, uchar status, short heading, short pitch);
	int SendAttemptConnect(uchar myPlayerNum, uchar networkVersion);
	int SendShare(uchar myPlayerNum, uchar shareTeam, uchar bShareUnits, float shareMetal, float shareEnergy);
	int SendSetShare(uchar myTeam, float metalShareFraction, float energyShareFraction);
	int SendSendPlayerStat();
	int SendPlayerStat(uchar myPlayerNum, const CPlayer::Statistics& currentStats);
	int SendGameOver();
	int SendMapErase(uchar myPlayerNum, short x, short z);
	int SendMapDrawLine(uchar myPlayerNum, short x1, short z1, short x2, short z2);
	int SendMapDrawPoint(uchar myPlayerNum, short x, short z, const std::string& label);
	int SendSyncRequest(int frameNum);
	int SendSyncResponse(uchar myPlayerNum, int frameNum, uint checksum);
	int SendSystemMessage(uchar myPlayerNum, const std::string& message);
	int SendStartPos(uchar myTeam, uchar ready, float x, float y, float z);
	int SendPlayerInfo(uchar myPlayerNum, float cpuUsage, int ping);
	int SendPlayerLeft(uchar myPlayerNum, uchar bIntended);
	int SendModName(const uint checksum, const std::string& modName);

	int GetMessageLength(const unsigned char* inbuf, int inbuflength) const;
	int ReadAhead(const unsigned char* inbuf, int inbuflength, int* que = NULL) const;

private:
	CDemoRecorder* record;
	CDemoReader* play;

	std::string scriptName;
	uint mapChecksum;
	std::string mapName;
	uint modChecksum;
	std::string modName;

	/// Bytes that don't make a complete net message yet (fragmented message).
	unsigned char fragbuf[netcode::NETWORK_BUFFER_SIZE];
	int fragbufLength;
};

extern CNetProtocol* net;

#endif
