#include "BaseNetProtocol.h"

#include "Rendering/InMapDraw.h"


CBaseNetProtocol::CBaseNetProtocol()
{
	RegisterMessage(NETMSG_QUIT, 1);
	RegisterMessage(NETMSG_NEWFRAME, 5);
	RegisterMessage(NETMSG_STARTPLAYING, 1);
	RegisterMessage(NETMSG_SETPLAYERNUM, 2);
	RegisterMessage(NETMSG_PLAYERNAME, -1);
	RegisterMessage(NETMSG_CHAT, -1);
	RegisterMessage(NETMSG_RANDSEED, 5);
	RegisterMessage(NETMSG_GAMEID, 17);
	RegisterMessage(NETMSG_COMMAND, -2);
	RegisterMessage(NETMSG_SELECT, -2);
	RegisterMessage(NETMSG_PAUSE, 3);
	RegisterMessage(NETMSG_AICOMMAND, -2);
	RegisterMessage(NETMSG_AICOMMANDS, -2);
	RegisterMessage(NETMSG_SCRIPT, -1);
	RegisterMessage(NETMSG_MAPNAME, -1);
	RegisterMessage(NETMSG_USER_SPEED, 6);
	RegisterMessage(NETMSG_INTERNAL_SPEED, 5);
	RegisterMessage(NETMSG_CPU_USAGE, 5);
	RegisterMessage(NETMSG_DIRECT_CONTROL, 2);
	RegisterMessage(NETMSG_DC_UPDATE, 7);
	RegisterMessage(NETMSG_ATTEMPTCONNECT, 3);
	RegisterMessage(NETMSG_SHARE, 12);
	RegisterMessage(NETMSG_SETSHARE, 11);
	RegisterMessage(NETMSG_SENDPLAYERSTAT, 1);
	RegisterMessage(NETMSG_PLAYERSTAT, 2+sizeof(CPlayer::Statistics));
	RegisterMessage(NETMSG_GAMEOVER, 1);
	RegisterMessage(NETMSG_MAPDRAW, -1);
	RegisterMessage(NETMSG_SYNCREQUEST, 5);
	RegisterMessage(NETMSG_SYNCRESPONSE, 10);
	RegisterMessage(NETMSG_SYSTEMMSG, -1);
	RegisterMessage(NETMSG_STARTPOS, 16);
	RegisterMessage(NETMSG_PLAYERINFO, 10);
	RegisterMessage(NETMSG_PLAYERLEFT, 3);
	RegisterMessage(NETMSG_MODNAME, -1);
	RegisterMessage(NETMSG_LUAMSG, -2);
}

CBaseNetProtocol::~CBaseNetProtocol()
{
	SendQuit();
}

void CBaseNetProtocol::RawSend(const uchar* data,const unsigned length)
{
	SendData(data, length);
}

//  NETMSG_QUIT             = 2,  //

void CBaseNetProtocol::SendQuit()
{
	SendData(NETMSG_QUIT);
}

void CBaseNetProtocol::SendQuit(unsigned playerNum)
{
	unsigned char a = NETMSG_QUIT;
	SendData(&a, 1, playerNum);
}

//  NETMSG_NEWFRAME         = 3,  // int frameNum;

void CBaseNetProtocol::SendNewFrame(int frameNum)
{
	SendData<int>(NETMSG_NEWFRAME, frameNum);
}

//  NETMSG_STARTPLAYING     = 4,  //

void CBaseNetProtocol::SendStartPlaying()
{
	SendData(NETMSG_STARTPLAYING);
}

//  NETMSG_SETPLAYERNUM     = 5,  // uchar myPlayerNum;

void CBaseNetProtocol::SendSetPlayerNum(uchar myPlayerNum, uchar connNumber)
{
	uchar msg[2] = {NETMSG_SETPLAYERNUM, myPlayerNum};
	SendData(msg, 2, connNumber);
}

//  NETMSG_PLAYERNAME       = 6,  // uchar myPlayerNum; std::string playerName;

void CBaseNetProtocol::SendPlayerName(uchar myPlayerNum, const std::string& playerName)
{
	SendSTLData<uchar, std::string>(NETMSG_PLAYERNAME, myPlayerNum, playerName);
}

//  NETMSG_CHAT             = 7,  // uchar myPlayerNum; std::string message;

void CBaseNetProtocol::SendChat(uchar myPlayerNum, const std::string& message)
{
	SendSTLData<uchar, std::string>(NETMSG_CHAT, myPlayerNum, message);
}

//  NETMSG_RANDSEED         = 8,  // uint randSeed;

void CBaseNetProtocol::SendRandSeed(uint randSeed)
{
	SendData<uint>(NETMSG_RANDSEED, randSeed);
}

//  NETMSG_GAMEID           = 9,  // char gameID[16];

void CBaseNetProtocol::SendGameID(const uchar* buf)
{
	uchar data[17];
	data[0] = NETMSG_GAMEID;
	memcpy(&data[1], buf, 16);
	SendData(data, 17);
}

//  NETMSG_COMMAND          = 11, // uchar myPlayerNum; int id; uchar options; std::vector<float> params;

void CBaseNetProtocol::SendCommand(uchar myPlayerNum, int id, uchar options, const std::vector<float>& params)
{
	SendSTLData<uchar, int, uchar, std::vector<float> >(NETMSG_COMMAND, myPlayerNum, id, options, params);
}

//  NETMSG_SELECT           = 12, // uchar myPlayerNum; std::vector<short> selectedUnitIDs;

void CBaseNetProtocol::SendSelect(uchar myPlayerNum, const std::vector<short>& selectedUnitIDs)
{
	SendSTLData<uchar, std::vector<short> >(NETMSG_SELECT, myPlayerNum, selectedUnitIDs);
}

//  NETMSG_PAUSE            = 13, // uchar playerNum, bPaused;

void CBaseNetProtocol::SendPause(uchar myPlayerNum, uchar bPaused)
{
	SendData<uchar, uchar>(NETMSG_PAUSE, myPlayerNum, bPaused);
}

//  NETMSG_AICOMMAND        = 14, // uchar myPlayerNum; short unitID; int id; uchar options; std::vector<float> params;

void CBaseNetProtocol::SendAICommand(uchar myPlayerNum, short unitID, int id, uchar options, const std::vector<float>& params)
{
	SendSTLData<uchar, short, int, uchar, std::vector<float> >(
			NETMSG_AICOMMAND, myPlayerNum, unitID, id, options, params);
}

//  NETMSG_AICOMMANDS       = 15, // uchar myPlayerNum;
	                              // short unitIDCount;  unitIDCount X short(unitID)
	                              // short commandCount; commandCount X { int id; uchar options; std::vector<float> params }

void CBaseNetProtocol::SendAICommands(uchar myPlayerNum, short unitIDCount, ...)
{
	//FIXME: needs special care; sits in CSelectedUnits::SendCommandsToUnits().
}

//  NETMSG_SCRIPT           = 16, // std::string scriptName;

void CBaseNetProtocol::SendScript(const std::string& newScriptName)
{
	SendSTLData<std::string> (NETMSG_SCRIPT, newScriptName);
}

//  NETMSG_MAPNAME          = 18, // uint checksum; std::string mapName;   (e.g. `SmallDivide.smf')

void CBaseNetProtocol::SendMapName(const uint checksum, const std::string& newMapName)
{
	return SendSTLData<uint, std::string>(NETMSG_MAPNAME, checksum, newMapName);
}

//  NETMSG_USER_SPEED       = 19, // uchar myPlayerNum, float userSpeed;
void CBaseNetProtocol::SendUserSpeed(uchar myPlayerNum, float userSpeed)
{
	SendData<uchar, float> (NETMSG_USER_SPEED, myPlayerNum, userSpeed);
}

//  NETMSG_INTERNAL_SPEED   = 20, // float internalSpeed;

void CBaseNetProtocol::SendInternalSpeed(float internalSpeed)
{
	SendData<float> (NETMSG_INTERNAL_SPEED, internalSpeed);
}

//  NETMSG_CPU_USAGE        = 21, // float cpuUsage;

void CBaseNetProtocol::SendCPUUsage(float cpuUsage)
{
	SendData<float> (NETMSG_CPU_USAGE, cpuUsage);
}

//  NETMSG_DIRECT_CONTROL   = 22, // uchar myPlayerNum;

void CBaseNetProtocol::SendDirectControl(uchar myPlayerNum)
{
	SendData<uchar> (NETMSG_DIRECT_CONTROL, myPlayerNum);
}

//  NETMSG_DC_UPDATE        = 23, // uchar myPlayerNum, status; short heading, pitch;

void CBaseNetProtocol::SendDirectControlUpdate(uchar myPlayerNum, uchar status, short heading, short pitch)
{
	SendData<uchar, uchar, short, short> (NETMSG_DC_UPDATE, myPlayerNum, status, heading, pitch);
}

//  NETMSG_ATTEMPTCONNECT   = 25, // uchar myPlayerNum, networkVersion;

void CBaseNetProtocol::SendAttemptConnect(uchar myPlayerNum, uchar networkVersion)
{
	SendData<uchar, uchar> (NETMSG_ATTEMPTCONNECT, myPlayerNum, networkVersion);
}

//  NETMSG_SHARE            = 26, // uchar myPlayerNum, shareTeam, bShareUnits; float shareMetal, shareEnergy;

void CBaseNetProtocol::SendShare(uchar myPlayerNum, uchar shareTeam, uchar bShareUnits, float shareMetal, float shareEnergy)
{
	SendData<uchar, uchar, uchar, float, float> (NETMSG_SHARE, myPlayerNum, shareTeam, bShareUnits, shareMetal, shareEnergy);
}

//  NETMSG_SETSHARE         = 27, // uchar myPlayerNum, uchar myTeam; float metalShareFraction, energyShareFraction;

void CBaseNetProtocol::SendSetShare(uchar myPlayerNum, uchar myTeam, float metalShareFraction, float energyShareFraction)
{
	SendData<uchar, uchar, float, float>(NETMSG_SETSHARE, myPlayerNum, myTeam, metalShareFraction, energyShareFraction);
}

//  NETMSG_SENDPLAYERSTAT   = 28, //

void CBaseNetProtocol::SendSendPlayerStat()
{
	SendData(NETMSG_SENDPLAYERSTAT);
}

//  NETMSG_PLAYERSTAT       = 29, // uchar myPlayerNum; CPlayer::Statistics currentStats;

void CBaseNetProtocol::SendPlayerStat(uchar myPlayerNum, const CPlayer::Statistics& currentStats)
{
	SendData<uchar, CPlayer::Statistics>(NETMSG_PLAYERSTAT, myPlayerNum, currentStats);
}

//  NETMSG_GAMEOVER         = 30, //

void CBaseNetProtocol::SendGameOver()
{
	SendData(NETMSG_GAMEOVER);
}

//  NETMSG_MAPDRAW          = 31, // uchar messageSize =  8, myPlayerNum, command = CInMapDraw::NET_ERASE; short x, z;
 // uchar messageSize = 12, myPlayerNum, command = CInMapDraw::NET_LINE; short x1, z1, x2, z2;
 // /*messageSize*/   uchar myPlayerNum, command = CInMapDraw::NET_POINT; short x, z; std::string label;

void CBaseNetProtocol::SendMapErase(uchar myPlayerNum, short x, short z)
{
	SendData<uchar, uchar, uchar, short, short>(NETMSG_MAPDRAW, 8, myPlayerNum, CInMapDraw::NET_ERASE, x, z);
}

void CBaseNetProtocol::SendMapDrawLine(uchar myPlayerNum, short x1, short z1, short x2, short z2)
{
	SendData<uchar, uchar, uchar, short, short, short, short>(NETMSG_MAPDRAW, 12, myPlayerNum, CInMapDraw::NET_LINE, x1, z1, x2, z2);
}

void CBaseNetProtocol::SendMapDrawPoint(uchar myPlayerNum, short x, short z, const std::string& label)
{
	SendSTLData<uchar, uchar, short, short, std::string>(NETMSG_MAPDRAW, myPlayerNum, CInMapDraw::NET_POINT, x, z, label);
}

//  NETMSG_SYNCREQUEST      = 32, // int frameNum;

void CBaseNetProtocol::SendSyncRequest(int frameNum)
{
	SendData<int>(NETMSG_SYNCREQUEST, frameNum);
}

//  NETMSG_SYNCRESPONSE     = 33, // uchar myPlayerNum; int frameNum; uint checksum;

void CBaseNetProtocol::SendSyncResponse(uchar myPlayerNum, int frameNum, uint checksum)
{
	SendData<uchar, int, uint>(NETMSG_SYNCRESPONSE, myPlayerNum, frameNum, checksum);
}

//  NETMSG_SYSTEMMSG        = 35, // uchar myPlayerNum; std::string message;

void CBaseNetProtocol::SendSystemMessage(uchar myPlayerNum, const std::string& message)
{
	SendSTLData<uchar, std::string>(NETMSG_SYSTEMMSG, myPlayerNum, message);
}

//  NETMSG_STARTPOS         = 36, // uchar myPlayerNum, uchar myTeam, ready /*0: not ready, 1: ready, 2: don't update readiness*/; float x, y, z;

void CBaseNetProtocol::SendStartPos(uchar myPlayerNum, uchar teamNum, uchar ready, float x, float y, float z)
{
	SendData<uchar, uchar, uchar, float, float, float>(NETMSG_STARTPOS, myPlayerNum, teamNum, ready, x, y, z);
}

//  NETMSG_PLAYERINFO       = 38, // uchar myPlayerNum; float cpuUsage; int ping /*in frames*/;
void CBaseNetProtocol::SendPlayerInfo(uchar myPlayerNum, float cpuUsage, int ping)
{
	SendData<uchar, float, int>(NETMSG_PLAYERINFO, myPlayerNum, cpuUsage, ping);
}

//  NETMSG_PLAYERLEFT       = 39, // uchar myPlayerNum, bIntended /*0: lost connection, 1: left*/;

void CBaseNetProtocol::SendPlayerLeft(uchar myPlayerNum, uchar bIntended)
{
	SendData<uchar, uchar>(NETMSG_PLAYERLEFT, myPlayerNum, bIntended);
}

//  NETMSG_MODNAME          = 40, // uint checksum; std::string modName;   (e.g. `XTA v8.1')

void CBaseNetProtocol::SendModName(const uint checksum, const std::string& newModName)
{
	return SendSTLData<uint, std::string> (NETMSG_MODNAME, checksum, newModName);
}

//  NETMSG_LUAMSG          = 50, // uchar myPlayerNum; std::string modName;   (e.g. `custom msg')

void CBaseNetProtocol::SendLuaMsg(uchar myPlayerNum, uchar script, uchar mode,
                                  const std::string& msg)
{
  // avoid the trailing '\0' processing
  const unsigned short msgLen = msg.size() + 6;
  const char* msgPtr = (char*)&msgLen;
  std::string data;
  data.push_back((char)NETMSG_LUAMSG);
  data.push_back(msgPtr[0]);
  data.push_back(msgPtr[1]);
  data.push_back(myPlayerNum);
  data.push_back(script);
  data.push_back(mode);
  data.append(msg);
  return SendData((const unsigned char*)data.data(), msgLen);    
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
 
