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

void CBaseNetProtocol::SendKeyFrame(int frameNum)
{
	PackPacket* packet = new PackPacket(5, NETMSG_KEYFRAME);
	*packet << frameNum;
	SendData(packet);
}

void CBaseNetProtocol::SendNewFrame()
{
	SendData(new PackPacket(1, NETMSG_NEWFRAME));
}


void CBaseNetProtocol::SendQuit()
{
	SendData(new PackPacket(1, NETMSG_QUIT));
}

void CBaseNetProtocol::SendQuit(unsigned playerNum)
{
	SendData(new PackPacket(5, NETMSG_QUIT), playerNum);
}

void CBaseNetProtocol::SendStartPlaying(unsigned countdown)
{
	PackPacket* packet = new PackPacket(5, NETMSG_STARTPLAYING);
	*packet << countdown;
	SendData(packet);
}

void CBaseNetProtocol::SendSetPlayerNum(uchar myPlayerNum, uchar connNumber)
{
	PackPacket* packet = new PackPacket(2, NETMSG_SETPLAYERNUM);
	*packet << myPlayerNum;
	SendData(packet, connNumber);
}

void CBaseNetProtocol::SendPlayerName(uchar myPlayerNum, const std::string& playerName)
{
	unsigned size = 3 + playerName.size() + 1;
	PackPacket* packet = new PackPacket(size, NETMSG_PLAYERNAME);
	*packet << static_cast<uchar>(size) << myPlayerNum << playerName;
	SendData(packet);
}

void CBaseNetProtocol::SendRandSeed(unsigned randSeed)
{
	PackPacket* packet = new PackPacket(5, NETMSG_RANDSEED);
	*packet << randSeed;
	SendData(packet);
}

void CBaseNetProtocol::SendRandSeed(unsigned randSeed, int toPlayer)
{
	PackPacket* packet = new PackPacket(5, NETMSG_RANDSEED);
	*packet << randSeed;
	SendData(packet, toPlayer);
}

// NETMSG_GAMEID = 9, char gameID[16];
void CBaseNetProtocol::SendGameID(const uchar* buf)
{
	PackPacket* packet = new PackPacket(17, NETMSG_GAMEID);
	memcpy(packet->GetWritingPos(), buf, 16);
	SendData(packet);
}


void CBaseNetProtocol::SendCommand(uchar myPlayerNum, int id, uchar options, const std::vector<float>& params)
{
	unsigned size = 9 + params.size() * sizeof(float);
	PackPacket* packet = new PackPacket(size, NETMSG_COMMAND);
	*packet << static_cast<unsigned short>(size) << myPlayerNum << id << options << params;
	SendData(packet);
}

void CBaseNetProtocol::SendSelect(uchar myPlayerNum, const std::vector<short>& selectedUnitIDs)
{
	unsigned size = 4 + selectedUnitIDs.size() * sizeof(short);
	PackPacket* packet = new PackPacket(size, NETMSG_SELECT);
	*packet << static_cast<unsigned short>(size) << myPlayerNum << selectedUnitIDs;
	SendData(packet);
}


void CBaseNetProtocol::SendPause(uchar myPlayerNum, uchar bPaused)
{
	PackPacket* packet = new PackPacket(3, NETMSG_PAUSE);
	*packet << myPlayerNum << bPaused;
	SendData(packet);
}



void CBaseNetProtocol::SendAICommand(uchar myPlayerNum, short unitID, int id, uchar options, const std::vector<float>& params)
{
	unsigned size = 11 + params.size() * sizeof(float);
	PackPacket* packet = new PackPacket(size, NETMSG_AICOMMAND);
	*packet << static_cast<unsigned short>(size) << myPlayerNum << unitID << id << options << params;
	SendData(packet);
}

void CBaseNetProtocol::SendAIShare(uchar myPlayerNum, uchar sourceTeam, uchar destTeam, float metal, float energy, const std::vector<short>& unitIDs)
{
	short totalNumBytes = (1 + sizeof(short)) + (3 + (2 * sizeof(float)) + (unitIDs.size() * sizeof(short)));

	PackPacket* packet = new PackPacket(totalNumBytes, NETMSG_AISHARE);
	*packet << totalNumBytes << myPlayerNum << sourceTeam << destTeam << metal << energy << unitIDs;
	SendData(packet);
}

void CBaseNetProtocol::SendUserSpeed(uchar myPlayerNum, float userSpeed)
{
	PackPacket* packet = new PackPacket(6, NETMSG_USER_SPEED);
	*packet << myPlayerNum << userSpeed;
	SendData(packet);
}

void CBaseNetProtocol::SendInternalSpeed(float internalSpeed)
{
	PackPacket* packet = new PackPacket(5, NETMSG_INTERNAL_SPEED);
	*packet << internalSpeed;
	SendData(packet);
}

void CBaseNetProtocol::SendCPUUsage(float cpuUsage)
{
	PackPacket* packet = new PackPacket(5, NETMSG_CPU_USAGE);
	*packet << cpuUsage;
	SendData(packet);
}


void CBaseNetProtocol::SendDirectControl(uchar myPlayerNum)
{
	PackPacket* packet = new PackPacket(2, NETMSG_DIRECT_CONTROL);
	*packet << myPlayerNum;
	SendData(packet);
}

void CBaseNetProtocol::SendDirectControlUpdate(uchar myPlayerNum, uchar status, short heading, short pitch)
{
	PackPacket* packet = new PackPacket(7, NETMSG_DC_UPDATE);
	*packet << myPlayerNum << status << heading << pitch;
	SendData(packet);
}


void CBaseNetProtocol::SendAttemptConnect(uchar myPlayerNum, uchar networkVersion)
{
	PackPacket* packet = new PackPacket(3, NETMSG_ATTEMPTCONNECT);
	*packet << myPlayerNum << networkVersion;
	SendData(packet);
}


void CBaseNetProtocol::SendShare(uchar myPlayerNum, uchar shareTeam, uchar bShareUnits, float shareMetal, float shareEnergy)
{
	PackPacket* packet = new PackPacket(12, NETMSG_SHARE);
	*packet << myPlayerNum << shareTeam << bShareUnits << shareMetal << shareEnergy;
	SendData(packet);
}

void CBaseNetProtocol::SendSetShare(uchar myPlayerNum, uchar myTeam, float metalShareFraction, float energyShareFraction)
{
	PackPacket* packet = new PackPacket(11, NETMSG_SETSHARE);
	*packet << myPlayerNum << myTeam << metalShareFraction << energyShareFraction;
	SendData(packet);
}


void CBaseNetProtocol::SendSendPlayerStat()
{
	SendData(new PackPacket(1, NETMSG_SENDPLAYERSTAT));
}

void CBaseNetProtocol::SendPlayerStat(uchar myPlayerNum, const CPlayer::Statistics& currentStats)
{
	PackPacket* packet = new PackPacket(2 + sizeof(CPlayer::Statistics), NETMSG_PLAYERSTAT);
	*packet << myPlayerNum << currentStats;
	SendData(packet);
}

void CBaseNetProtocol::SendGameOver()
{
	SendData(new PackPacket(1, NETMSG_GAMEOVER));
}

// NETMSG_MAPDRAW = 31, uchar messageSize =  8, myPlayerNum, command = CInMapDraw::NET_ERASE; short x, z;
void CBaseNetProtocol::SendMapErase(uchar myPlayerNum, short x, short z)
{
	PackPacket* packet = new PackPacket(8, NETMSG_MAPDRAW);
	*packet << static_cast<uchar>(8) << myPlayerNum << static_cast<uchar>(CInMapDraw::NET_ERASE) << x << z;
	SendData(packet);
}

// NETMSG_MAPDRAW = 31, uchar messageSize = 12, myPlayerNum, command = CInMapDraw::NET_LINE; short x1, z1, x2, z2;
void CBaseNetProtocol::SendMapDrawLine(uchar myPlayerNum, short x1, short z1, short x2, short z2)
{
	PackPacket* packet = new PackPacket(12, NETMSG_MAPDRAW);
	*packet << static_cast<uchar>(12) << myPlayerNum << static_cast<uchar>(CInMapDraw::NET_LINE) << x1 << z1 << x2 << z2;
	SendData(packet);
}

// NETMSG_MAPDRAW = 31, uchar messageSize, uchar myPlayerNum, command = CInMapDraw::NET_POINT; short x, z; std::string label;
void CBaseNetProtocol::SendMapDrawPoint(uchar myPlayerNum, short x, short z, const std::string& label)
{
	unsigned size = 8 + label.size() + 1;
	PackPacket* packet = new PackPacket(size, NETMSG_MAPDRAW);
	*packet << static_cast<uchar>(size) << myPlayerNum << static_cast<uchar>(CInMapDraw::NET_POINT) << x << z << label;
	SendData(packet);
}

void CBaseNetProtocol::SendSyncRequest(int frameNum)
{
	PackPacket* packet = new PackPacket(5, NETMSG_SYNCREQUEST);
	*packet << frameNum;
	SendData(packet);
}

void CBaseNetProtocol::SendSyncResponse(uchar myPlayerNum, int frameNum, uint checksum)
{
	PackPacket* packet = new PackPacket(10, NETMSG_SYNCRESPONSE);
	*packet << myPlayerNum << frameNum << checksum;
	SendData(packet);
}

void CBaseNetProtocol::SendSystemMessage(uchar myPlayerNum, const std::string& message)
{
	unsigned size = 3 + message.size() + 1;
	PackPacket* packet = new PackPacket(size, NETMSG_SYSTEMMSG);
	*packet << static_cast<uchar>(size) << myPlayerNum << message;
	SendData(packet);
}

void CBaseNetProtocol::SendStartPos(uchar myPlayerNum, uchar teamNum, uchar ready, float x, float y, float z)
{
	PackPacket* packet = new PackPacket(16, NETMSG_STARTPOS);
	*packet << myPlayerNum << teamNum << ready << x << y << z;
	SendData(packet);
}

void CBaseNetProtocol::SendPlayerInfo(uchar myPlayerNum, float cpuUsage, int ping)
{
	PackPacket* packet = new PackPacket(10, NETMSG_PLAYERINFO);
	*packet << myPlayerNum << cpuUsage << ping;
	SendData(packet);
}

void CBaseNetProtocol::SendPlayerLeft(uchar myPlayerNum, uchar bIntended)
{
	PackPacket* packet = new PackPacket(3, NETMSG_PLAYERLEFT);
	*packet << myPlayerNum << bIntended;
	SendData(packet);
}

// NETMSG_LUAMSG = 50, uchar myPlayerNum; std::string modName; (e.g. `custom msg')
void CBaseNetProtocol::SendLuaMsg(uchar myPlayerNum, uchar script, uchar mode,
                                  const std::string& msg)
{
	unsigned short size = 6 + msg.size()+1;
	PackPacket* packet = new PackPacket(size, NETMSG_LUAMSG);
	*packet << size << myPlayerNum << script << mode << msg;
	SendData(packet);
}

void CBaseNetProtocol::SendSelfD(uchar myPlayerNum)
{
	PackPacket* packet = new PackPacket(4, NETMSG_TEAM);
	*packet << myPlayerNum << static_cast<uchar>(TEAMMSG_SELFD) << static_cast<uchar>(0);
	SendData(packet);
}

void CBaseNetProtocol::SendGiveAwayEverything(uchar myPlayerNum, uchar giveTo)
{
	PackPacket* packet = new PackPacket(4, NETMSG_TEAM);
	*packet << myPlayerNum << static_cast<uchar>(TEAMMSG_GIVEAWAY) << giveTo;
	SendData(packet);
}

void CBaseNetProtocol::SendResign(uchar myPlayerNum)
{
	PackPacket* packet = new PackPacket(4, NETMSG_TEAM);
	*packet << myPlayerNum << static_cast<uchar>(TEAMMSG_RESIGN) << static_cast<uchar>(0);
	SendData(packet);
}

void CBaseNetProtocol::SendJoinTeam(uchar myPlayerNum, uchar wantedTeamNum)
{
	PackPacket* packet = new PackPacket(4, NETMSG_TEAM);
	*packet << myPlayerNum << static_cast<uchar>(TEAMMSG_JOIN_TEAM) << wantedTeamNum;
	SendData(packet);
}

void CBaseNetProtocol::SendTeamDied(uchar myPlayerNum, uchar whichTeam)
{
	PackPacket* packet = new PackPacket(4, NETMSG_TEAM);
	*packet << myPlayerNum << static_cast<uchar>(TEAMMSG_TEAM_DIED) << whichTeam;
	SendData(packet);
}

void CBaseNetProtocol::SendSetAllied(uchar myPlayerNum, uchar whichAllyTeam, uchar state)
{
	PackPacket* packet = new PackPacket(4, NETMSG_ALLIANCE);
	*packet << myPlayerNum << whichAllyTeam << state;
	SendData(packet);
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
