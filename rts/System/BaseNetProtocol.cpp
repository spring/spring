#include "BaseNetProtocol.h"

#include "Rendering/InMapDraw.h"
#include "Net/PackPacket.h"
using netcode::PackPacket;

CBaseNetProtocol::CBaseNetProtocol()
{
  // RegisterMessage() length parameter:
  //   > 0:  if its fixed length
  //   < 0:  means the next x bytes represent the length

	RegisterMessage(NETMSG_KEYFRAME, 5);
	RegisterMessage(NETMSG_NEWFRAME, 1);
	RegisterMessage(NETMSG_QUIT, 1);
	RegisterMessage(NETMSG_STARTPLAYING, 5);
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
	RegisterMessage(NETMSG_AISHARE, -2);

	RegisterMessage(NETMSG_USER_SPEED, 6);
	RegisterMessage(NETMSG_INTERNAL_SPEED, 5);
	RegisterMessage(NETMSG_CPU_USAGE, 5);
	RegisterMessage(NETMSG_DIRECT_CONTROL, 2);
	RegisterMessage(NETMSG_DC_UPDATE, 7);
	RegisterMessage(NETMSG_ATTEMPTCONNECT, 3);
	RegisterMessage(NETMSG_SHARE, 12);
	RegisterMessage(NETMSG_SETSHARE, 11);
	RegisterMessage(NETMSG_SENDPLAYERSTAT, 1);
	RegisterMessage(NETMSG_PLAYERSTAT, 2 + sizeof(CPlayer::Statistics));
	RegisterMessage(NETMSG_GAMEOVER, 1);
	RegisterMessage(NETMSG_MAPDRAW, -1);
	RegisterMessage(NETMSG_SYNCREQUEST, 5);
	RegisterMessage(NETMSG_SYNCRESPONSE, 10);
	RegisterMessage(NETMSG_SYSTEMMSG, -1);
	RegisterMessage(NETMSG_STARTPOS, 16);
	RegisterMessage(NETMSG_PLAYERINFO, 10);
	RegisterMessage(NETMSG_PLAYERLEFT, 3);
	RegisterMessage(NETMSG_LUAMSG, -2);
	RegisterMessage(NETMSG_TEAM, 4);
	RegisterMessage(NETMSG_GAMEDATA, -2);
	RegisterMessage(NETMSG_ALLIANCE, 4);
	RegisterMessage(NETMSG_CCOMMAND, -2);
}

CBaseNetProtocol::~CBaseNetProtocol()
{
	SendQuit();
}

void CBaseNetProtocol::RawSend(const uchar* data, const unsigned length)
{
	SendData(data, length);
}

void CBaseNetProtocol::SendKeyFrame(int frameNum)
{
	SendData<int>(NETMSG_KEYFRAME, frameNum);
}

void CBaseNetProtocol::SendNewFrame()
{
	unsigned char msg = NETMSG_NEWFRAME;
	SendData(&msg, 1);
}


void CBaseNetProtocol::SendQuit()
{
	SendData(NETMSG_QUIT);
}

void CBaseNetProtocol::SendQuit(unsigned playerNum)
{
	unsigned char a = NETMSG_QUIT;
	SendData(&a, 1, playerNum);
}

void CBaseNetProtocol::SendStartPlaying(unsigned countdown)
{
	SendData<unsigned>(NETMSG_STARTPLAYING, countdown);
}

void CBaseNetProtocol::SendSetPlayerNum(uchar myPlayerNum, uchar connNumber)
{
	uchar msg[2] = {NETMSG_SETPLAYERNUM, myPlayerNum};
	SendData(msg, 2, connNumber);
}

void CBaseNetProtocol::SendPlayerName(uchar myPlayerNum, const std::string& playerName)
{
	unsigned size = 3 + playerName.size() + 1;
	PackPacket* packet = new PackPacket(size);
	*packet << static_cast<uchar>(NETMSG_PLAYERNAME) << static_cast<uchar>(size) << myPlayerNum << playerName;
	SendData(packet);
}

void CBaseNetProtocol::SendRandSeed(uint randSeed)
{
	SendData<uint>(NETMSG_RANDSEED, randSeed);
}

void CBaseNetProtocol::SendRandSeed(uint randSeed, int toPlayer)
{
	uchar data[5] = {NETMSG_RANDSEED};
	*(int*)(data + 1) = randSeed;
	SendData(data, 5, toPlayer);
}

// NETMSG_GAMEID = 9, char gameID[16];
void CBaseNetProtocol::SendGameID(const uchar* buf)
{
	uchar data[17];
	data[0] = NETMSG_GAMEID;
	memcpy(&data[1], buf, 16);
	SendData(data, 17);
}


void CBaseNetProtocol::SendCommand(uchar myPlayerNum, int id, uchar options, const std::vector<float>& params)
{
	unsigned size = 9 + params.size() * sizeof(float);
	PackPacket* packet = new PackPacket(size);
	*packet << static_cast<uchar>(NETMSG_COMMAND) << static_cast<unsigned short>(size) << myPlayerNum << id << options << params;
	SendData(packet);
}

void CBaseNetProtocol::SendSelect(uchar myPlayerNum, const std::vector<short>& selectedUnitIDs)
{
	unsigned size = 4 + selectedUnitIDs.size() * sizeof(short);
	PackPacket* packet = new PackPacket(size);
	*packet << static_cast<uchar>(NETMSG_SELECT) << static_cast<unsigned short>(size) << myPlayerNum << selectedUnitIDs;
	SendData(packet);
}


void CBaseNetProtocol::SendPause(uchar myPlayerNum, uchar bPaused)
{
	SendData<uchar, uchar>(NETMSG_PAUSE, myPlayerNum, bPaused);
}



void CBaseNetProtocol::SendAICommand(uchar myPlayerNum, short unitID, int id, uchar options, const std::vector<float>& params)
{
	unsigned size = 11 + params.size() * sizeof(float);
	PackPacket* packet = new PackPacket(size);
	*packet << static_cast<uchar>(NETMSG_AICOMMAND) << static_cast<unsigned short>(size) << myPlayerNum << unitID << id << options << params;
	SendData(packet);
}

void CBaseNetProtocol::SendAICommands(uchar myPlayerNum, short unitIDCount, ...)
{
	// FIXME: needs special care; sits in CSelectedUnits::SendCommandsToUnits().
}

void CBaseNetProtocol::SendAIShare(uchar myPlayerNum, uchar sourceTeam, uchar destTeam, float metal, float energy, const std::vector<short>& unitIDs)
{
	short totalNumBytes = (1 + sizeof(short)) + (3 + (2 * sizeof(float)) + (unitIDs.size() * sizeof(short)));

	PackPacket* packet = new PackPacket(totalNumBytes);
	*packet << static_cast<uchar>(NETMSG_AISHARE) << totalNumBytes << myPlayerNum << sourceTeam << destTeam << metal << energy << unitIDs;
	SendData(packet);
}



void CBaseNetProtocol::SendUserSpeed(uchar myPlayerNum, float userSpeed)
{
	SendData<uchar, float> (NETMSG_USER_SPEED, myPlayerNum, userSpeed);
}

void CBaseNetProtocol::SendInternalSpeed(float internalSpeed)
{
	SendData<float> (NETMSG_INTERNAL_SPEED, internalSpeed);
}

void CBaseNetProtocol::SendCPUUsage(float cpuUsage)
{
	SendData<float> (NETMSG_CPU_USAGE, cpuUsage);
}


void CBaseNetProtocol::SendDirectControl(uchar myPlayerNum)
{
	SendData<uchar> (NETMSG_DIRECT_CONTROL, myPlayerNum);
}

void CBaseNetProtocol::SendDirectControlUpdate(uchar myPlayerNum, uchar status, short heading, short pitch)
{
	SendData<uchar, uchar, short, short> (NETMSG_DC_UPDATE, myPlayerNum, status, heading, pitch);
}


void CBaseNetProtocol::SendAttemptConnect(uchar myPlayerNum, uchar networkVersion)
{
	SendData<uchar, uchar> (NETMSG_ATTEMPTCONNECT, myPlayerNum, networkVersion);
}


void CBaseNetProtocol::SendShare(uchar myPlayerNum, uchar shareTeam, uchar bShareUnits, float shareMetal, float shareEnergy)
{
	SendData<uchar, uchar, uchar, float, float> (NETMSG_SHARE, myPlayerNum, shareTeam, bShareUnits, shareMetal, shareEnergy);
}

void CBaseNetProtocol::SendSetShare(uchar myPlayerNum, uchar myTeam, float metalShareFraction, float energyShareFraction)
{
	SendData<uchar, uchar, float, float>(NETMSG_SETSHARE, myPlayerNum, myTeam, metalShareFraction, energyShareFraction);
}


void CBaseNetProtocol::SendSendPlayerStat()
{
	SendData(NETMSG_SENDPLAYERSTAT);
}

void CBaseNetProtocol::SendPlayerStat(uchar myPlayerNum, const CPlayer::Statistics& currentStats)
{
	SendData<uchar, CPlayer::Statistics>(NETMSG_PLAYERSTAT, myPlayerNum, currentStats);
}


void CBaseNetProtocol::SendGameOver()
{
	SendData(NETMSG_GAMEOVER);
}



// NETMSG_MAPDRAW = 31, uchar messageSize =  8, myPlayerNum, command = CInMapDraw::NET_ERASE; short x, z;
void CBaseNetProtocol::SendMapErase(uchar myPlayerNum, short x, short z)
{
	SendData<uchar, uchar, uchar, short, short>(NETMSG_MAPDRAW, 8, myPlayerNum, CInMapDraw::NET_ERASE, x, z);
}

// NETMSG_MAPDRAW = 31, uchar messageSize = 12, myPlayerNum, command = CInMapDraw::NET_LINE; short x1, z1, x2, z2;
void CBaseNetProtocol::SendMapDrawLine(uchar myPlayerNum, short x1, short z1, short x2, short z2)
{
	SendData<uchar, uchar, uchar, short, short, short, short>(NETMSG_MAPDRAW, 12, myPlayerNum, CInMapDraw::NET_LINE, x1, z1, x2, z2);
}

// NETMSG_MAPDRAW = 31, uchar messageSize, uchar myPlayerNum, command = CInMapDraw::NET_POINT; short x, z; std::string label;
void CBaseNetProtocol::SendMapDrawPoint(uchar myPlayerNum, short x, short z, const std::string& label)
{
	unsigned size = 8 + label.size() + 1;
	PackPacket* packet = new PackPacket(size);
	*packet << static_cast<uchar>(NETMSG_MAPDRAW) << static_cast<uchar>(size) << myPlayerNum << static_cast<uchar>(CInMapDraw::NET_POINT) << x << z << label;
	SendData(packet);
}



void CBaseNetProtocol::SendSyncRequest(int frameNum)
{
	SendData<int>(NETMSG_SYNCREQUEST, frameNum);
}

void CBaseNetProtocol::SendSyncResponse(uchar myPlayerNum, int frameNum, uint checksum)
{
	SendData<uchar, int, uint>(NETMSG_SYNCRESPONSE, myPlayerNum, frameNum, checksum);
}

void CBaseNetProtocol::SendSystemMessage(uchar myPlayerNum, const std::string& message)
{
	unsigned size = 3 + message.size() + 1;
	PackPacket* packet = new PackPacket(size);
	*packet << static_cast<uchar>(NETMSG_SYSTEMMSG) << static_cast<uchar>(size) << myPlayerNum << message;
	SendData(packet);
}

void CBaseNetProtocol::SendStartPos(uchar myPlayerNum, uchar teamNum, uchar ready, float x, float y, float z)
{
	SendData<uchar, uchar, uchar, float, float, float>(NETMSG_STARTPOS, myPlayerNum, teamNum, ready, x, y, z);
}

void CBaseNetProtocol::SendPlayerInfo(uchar myPlayerNum, float cpuUsage, int ping)
{
	SendData<uchar, float, int>(NETMSG_PLAYERINFO, myPlayerNum, cpuUsage, ping);
}

void CBaseNetProtocol::SendPlayerLeft(uchar myPlayerNum, uchar bIntended)
{
	SendData<uchar, uchar>(NETMSG_PLAYERLEFT, myPlayerNum, bIntended);
}

// NETMSG_LUAMSG = 50, uchar myPlayerNum; std::string modName; (e.g. `custom msg')
void CBaseNetProtocol::SendLuaMsg(uchar myPlayerNum, uchar script, uchar mode,
                                  const std::string& msg)
{
	// avoid the trailing '\0' processing
	const unsigned short msgLen = msg.size() + 6;
	const char* msgPtr = (char*) &msgLen;
	std::string data;
	data.push_back((char)NETMSG_LUAMSG);
	data.push_back(msgPtr[0]);
	data.push_back(msgPtr[1]);
	data.push_back(myPlayerNum);
	data.push_back(script);
	data.push_back(mode);
	data.append(msg);
	SendData((const unsigned char*) data.data(), msgLen);
}

void CBaseNetProtocol::SendSelfD(uchar myPlayerNum)
{
	unsigned char msg[4] = {NETMSG_TEAM, myPlayerNum, TEAMMSG_SELFD, 0};
	SendData(msg, 4);
}

void CBaseNetProtocol::SendGiveAwayEverything(uchar myPlayerNum, uchar giveTo)
{
	unsigned char msg[4] = {NETMSG_TEAM, myPlayerNum, TEAMMSG_GIVEAWAY, giveTo};
	SendData(msg, 4);
}

void CBaseNetProtocol::SendResign(uchar myPlayerNum)
{
	unsigned char msg[4] = {NETMSG_TEAM, myPlayerNum, TEAMMSG_RESIGN, 0};
	SendData(msg, 4);
}

void CBaseNetProtocol::SendJoinTeam(uchar myPlayerNum, uchar wantedTeamNum)
{
	unsigned char msg[4] = {NETMSG_TEAM, myPlayerNum, TEAMMSG_JOIN_TEAM, wantedTeamNum};
	SendData(msg, 4);
}

void CBaseNetProtocol::SendTeamDied(uchar myPlayerNum, uchar whichTeam)
{
	unsigned char msg[4] = {NETMSG_TEAM, myPlayerNum, TEAMMSG_TEAM_DIED, whichTeam};
	SendData(msg, 4);
}

void CBaseNetProtocol::SendSetAllied(uchar myPlayerNum, uchar whichAllyTeam, uchar state)
{
	unsigned char msg[4] = {NETMSG_ALLIANCE, myPlayerNum, whichAllyTeam, state};
	SendData(msg, 4);
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
