#include "StdAfx.h"
#include "NetProtocol.h"
#include "Rendering/InMapDraw.h"

// 	NETMSG_HELLO            = 1,  //

int CNetProtocol::SendHello()
{
	return SendData(NETMSG_HELLO);
}

//  NETMSG_QUIT             = 2,  //

int CNetProtocol::SendQuit()
{
	return SendData(NETMSG_QUIT);
}

//  NETMSG_NEWFRAME         = 3,  // int frameNum;

int CNetProtocol::SendNewFrame(int frameNum)
{
	return SendData<int>(NETMSG_NEWFRAME, frameNum);
}

//  NETMSG_STARTPLAYING     = 4,  //

int CNetProtocol::SendStartPlaying()
{
	return SendData(NETMSG_STARTPLAYING);
}

//  NETMSG_SETPLAYERNUM     = 5,  // uchar myPlayerNum;

int CNetProtocol::SendSetPlayerNum(uchar myPlayerNum)
{
	return SendData<uchar>(NETMSG_SETPLAYERNUM, myPlayerNum);
}

//  NETMSG_PLAYERNAME       = 6,  // uchar myPlayerNum; std::string playerName;

int CNetProtocol::SendPlayerName(uchar myPlayerNum, const std::string& playerName)
{
	return SendSTLData<uchar, std::string>(NETMSG_PLAYERNAME, myPlayerNum, playerName);
}

//  NETMSG_CHAT             = 7,  // uchar myPlayerNum; std::string message;

int CNetProtocol::SendChat(uchar myPlayerNum, const std::string& message)
{
	return SendSTLData<uchar, std::string>(NETMSG_CHAT, myPlayerNum, message);
}

//  NETMSG_RANDSEED         = 8,  // uint randSeed;

int CNetProtocol::SendRandSeed(uint randSeed)
{
	return SendData<uint>(NETMSG_RANDSEED, randSeed);
}

//  NETMSG_COMMAND          = 11, // uchar myPlayerNum; int id; uchar options; std::vector<float> params;

int CNetProtocol::SendCommand(uchar myPlayerNum, int id, uchar options, const std::vector<float>& params)
{
	return SendSTLData<uchar, int, uchar, std::vector<float> >(NETMSG_COMMAND, myPlayerNum, id, options, params);
}

//  NETMSG_SELECT           = 12, // uchar myPlayerNum; std::vector<short> selectedUnitIDs;

int CNetProtocol::SendSelect(uchar myPlayerNum, const std::vector<short>& selectedUnitIDs)
{
	return SendSTLData<uchar, std::vector<short> >(NETMSG_SELECT, myPlayerNum, selectedUnitIDs);
}

//  NETMSG_PAUSE            = 13, // uchar playerNum, bPaused;

int CNetProtocol::SendPause(uchar myPlayerNum, uchar bPaused)
{
	return SendData<uchar, uchar>(NETMSG_PAUSE, myPlayerNum, bPaused);
}

//  NETMSG_AICOMMAND        = 14, // uchar myPlayerNum; short unitID; int id; uchar options; std::vector<float> params;

int CNetProtocol::SendAICommand(uchar myPlayerNum, short unitID, int id, uchar options, const std::vector<float>& params)
{
	return SendSTLData<uchar, short, int, uchar, std::vector<float> >(
			NETMSG_AICOMMAND, myPlayerNum, unitID, id, options, params);
}

//  NETMSG_AICOMMANDS       = 15, // uchar myPlayerNum;
	                              // short unitIDCount;  unitIDCount X short(unitID)
	                              // short commandCount; commandCount X { int id; uchar options; std::vector<float> params }

int CNetProtocol::SendAICommands(uchar myPlayerNum, short unitIDCount, ...)
{
	return 0;
	//FIXME: needs special care; sits in CSelectedUnits::SendCommandsToUnits().
}

//  NETMSG_SCRIPT           = 16, // std::string scriptName;

int CNetProtocol::SendScript(const std::string& scriptName)
{
	return SendSTLData<std::string> (NETMSG_SCRIPT, scriptName);
}

//  NETMSG_MAPNAME          = 18, // uint checksum; std::string mapName;   (e.g. `SmallDivide.smf')

int CNetProtocol::SendMapName(uint checksum, std::string mapName)
{
	return SendSTLData<uint, std::string>(NETMSG_MAPNAME, checksum, mapName);
}

//  NETMSG_USER_SPEED       = 19, // float userSpeed;

int CNetProtocol::SendUserSpeed(float userSpeed)
{
	return SendData<float> (NETMSG_USER_SPEED, userSpeed);
}

//  NETMSG_INTERNAL_SPEED   = 20, // float internalSpeed;

int CNetProtocol::SendInternalSpeed(float internalSpeed)
{
	return SendData<float> (NETMSG_INTERNAL_SPEED, internalSpeed);
}

//  NETMSG_CPU_USAGE        = 21, // float cpuUsage;

int CNetProtocol::SendCPUUsage(float cpuUsage)
{
	return SendData<float> (NETMSG_CPU_USAGE, cpuUsage);
}

//  NETMSG_DIRECT_CONTROL   = 22, // uchar myPlayerNum;

int CNetProtocol::SendDirectControl(uchar myPlayerNum)
{
	return SendData<uchar> (NETMSG_DIRECT_CONTROL, myPlayerNum);
}

//  NETMSG_DC_UPDATE        = 23, // uchar myPlayerNum, status; short heading, pitch;

int CNetProtocol::SendDirectControlUpdate(uchar myPlayerNum, uchar status, short heading, short pitch)
{
	return SendData<uchar, uchar, short, short> (NETMSG_DC_UPDATE, myPlayerNum, status, heading, pitch);
}

//  NETMSG_ATTEMPTCONNECT   = 25, // uchar myPlayerNum, networkVersion;

int CNetProtocol::SendAttemptConnect(uchar myPlayerNum, uchar networkVersion)
{
	return SendData<uchar, uchar> (NETMSG_ATTEMPTCONNECT, myPlayerNum, networkVersion);
}

//  NETMSG_SHARE            = 26, // uchar myPlayerNum, shareTeam, bShareUnits; float shareMetal, shareEnergy;

int CNetProtocol::SendShare(uchar myPlayerNum, uchar shareTeam, uchar bShareUnits, float shareMetal, float shareEnergy)
{
	return SendData<uchar, uchar, uchar, float, float> (
			NETMSG_SHARE, myPlayerNum, shareTeam, bShareUnits, shareMetal, shareEnergy);
}

//  NETMSG_SETSHARE         = 27, // uchar myTeam; float metalShareFraction, energyShareFraction;

int CNetProtocol::SendSetShare(uchar myTeam, float metalShareFraction, float energyShareFraction)
{
	return SendData<uchar, float, float>(NETMSG_SETSHARE, myTeam, metalShareFraction, energyShareFraction);
}

//  NETMSG_SENDPLAYERSTAT   = 28, //

int CNetProtocol::SendSendPlayerStat()
{
	return SendData(NETMSG_SENDPLAYERSTAT);
}

//  NETMSG_PLAYERSTAT       = 29, // uchar myPlayerNum; CPlayer::Statistics currentStats;

int CNetProtocol::SendPlayerStat(uchar myPlayerNum, const CPlayer::Statistics& currentStats)
{
	return SendData<uchar, CPlayer::Statistics>(NETMSG_PLAYERSTAT, myPlayerNum, currentStats);
}

//  NETMSG_GAMEOVER         = 30, //

int CNetProtocol::SendGameOver()
{
	return SendData(NETMSG_GAMEOVER);
}

//  NETMSG_MAPDRAW          = 31, // uchar messageSize =  8, myPlayerNum, command = CInMapDraw::NET_ERASE; short x, z;
 // uchar messageSize = 12, myPlayerNum, command = CInMapDraw::NET_LINE; short x1, z1, x2, z2;
 // /*messageSize*/   uchar myPlayerNum, command = CInMapDraw::NET_POINT; short x, z; std::string label;

int CNetProtocol::SendMapErase(uchar myPlayerNum, short x, short z)
{
	return SendData<uchar, uchar, uchar, short, short>(
	                NETMSG_MAPDRAW, 8, myPlayerNum, CInMapDraw::NET_ERASE, x, z);
}

int CNetProtocol::SendMapDrawLine(uchar myPlayerNum, short x1, short z1, short x2, short z2)
{
	return SendData<uchar, uchar, uchar, short, short, short, short>(
	                NETMSG_MAPDRAW, 12, myPlayerNum, CInMapDraw::NET_LINE, x1, z1, x2, z2);
}

int CNetProtocol::SendMapDrawPoint(uchar myPlayerNum, short x, short z, const std::string& label)
{
	return SendSTLData<uchar, uchar, short, short, std::string>(
	                   NETMSG_MAPDRAW, myPlayerNum, CInMapDraw::NET_POINT, x, z, label);
}

//  NETMSG_SYNCREQUEST      = 32, // int frameNum;

int CNetProtocol::SendSyncRequest(int frameNum)
{
	return SendData<int>(NETMSG_SYNCREQUEST, frameNum);
}

//  NETMSG_SYNCRESPONSE     = 33, // uchar myPlayerNum; int frameNum; uint checksum;

int CNetProtocol::SendSyncResponse(uchar myPlayerNum, int frameNum, uint checksum)
{
	return SendData<uchar, int, uint>(NETMSG_SYNCRESPONSE, myPlayerNum, frameNum, checksum);
}

//  NETMSG_SYSTEMMSG        = 35, // uchar myPlayerNum; std::string message;

int CNetProtocol::SendSystemMessage(uchar myPlayerNum, const std::string& message)
{
	return SendSTLData<uchar, std::string>(NETMSG_SYSTEMMSG, myPlayerNum, message);
}

//  NETMSG_STARTPOS         = 36, // uchar myTeam, ready /*0: not ready, 1: ready, 2: don't update readiness*/; float x, y, z;

int CNetProtocol::SendStartPos(uchar myTeam, uchar ready, float x, float y, float z)
{
	return SendData<uchar, uchar, float, float, float>(NETMSG_STARTPOS, myTeam, ready, x, y, z);
}

//  NETMSG_PLAYERINFO       = 38, // uchar myPlayerNum; float cpuUsage; int ping /*in frames*/;

int CNetProtocol::SendPlayerInfo(uchar myPlayerNum, float cpuUsage, int ping)
{
	return SendData<uchar, float, int>(NETMSG_PLAYERINFO, myPlayerNum, cpuUsage, ping);
}

//  NETMSG_PLAYERLEFT       = 39, // uchar myPlayerNum, bIntended /*0: lost connection, 1: left*/;

int CNetProtocol::SendPlayerLeft(uchar myPlayerNum, uchar bIntended)
{
	return SendData<uchar, uchar>(NETMSG_PLAYERLEFT, myPlayerNum, bIntended);
}

//  NETMSG_MODNAME          = 40, // uint checksum; std::string modName;   (e.g. `XTA v8.1')

int CNetProtocol::SendModName(uint checksum, std::string modName)
{
	return SendSTLData<uint, std::string> (NETMSG_MODNAME, checksum, modName);
}

/* FIXME: add these:

#ifdef SYNCDEBUG
 NETMSG_SD_CHKREQUEST    = 41,
 NETMSG_SD_CHKRESPONSE   = 42,
 NETMSG_SD_BLKREQUEST    = 43,
 NETMSG_SD_BLKRESPONSE   = 44,
 NETMSG_SD_RESET         = 45,
#endif // SYNCDEBUG

*/

CNetProtocol* net=0;
CNetProtocol* serverNet=0;
