/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "BaseNetProtocol.h"

#include <boost/cstdint.hpp>
#include "mmgr.h"

#include "Game/PlayerStatistics.h"
#include "Sim/Misc/Team.h"
#include "Net/RawPacket.h"
#include "Net/PackPacket.h"
#include "Net/ProtocolDef.h"
#if defined(_MSC_VER)
#include "System.h" // for uint16_t (and possibly other types)
#endif

using netcode::PackPacket;
typedef boost::shared_ptr<const netcode::RawPacket> PacketType;

CBaseNetProtocol& CBaseNetProtocol::Get()
{
	static CBaseNetProtocol instance;
	return instance;
}

PacketType CBaseNetProtocol::SendKeyFrame(int frameNum)
{
	PackPacket* packet = new PackPacket(5, NETMSG_KEYFRAME);
	*packet << frameNum;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendNewFrame()
{
	return PacketType(new PackPacket(1, NETMSG_NEWFRAME));
}


PacketType CBaseNetProtocol::SendQuit(const std::string& reason)
{
	unsigned size = 3 + reason.size() + 1;
	PackPacket* packet = new PackPacket(size, NETMSG_QUIT);
	*packet << static_cast<boost::uint16_t>(size) << reason;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendStartPlaying(unsigned countdown)
{
	PackPacket* packet = new PackPacket(5, NETMSG_STARTPLAYING);
	*packet << countdown;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSetPlayerNum(uchar myPlayerNum)
{
	PackPacket* packet = new PackPacket(2, NETMSG_SETPLAYERNUM);
	*packet << myPlayerNum;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendPlayerName(uchar myPlayerNum, const std::string& playerName)
{
	unsigned size = 3 + playerName.size() + 1;
	PackPacket* packet = new PackPacket(size, NETMSG_PLAYERNAME);
	*packet << static_cast<uchar>(size) << myPlayerNum << playerName;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendRandSeed(unsigned randSeed)
{
	PackPacket* packet = new PackPacket(5, NETMSG_RANDSEED);
	*packet << randSeed;
	return PacketType(packet);
}

// NETMSG_GAMEID = 9, char gameID[16];
PacketType CBaseNetProtocol::SendGameID(const uchar* buf)
{
	PackPacket* packet = new PackPacket(17, NETMSG_GAMEID);
	memcpy(packet->GetWritingPos(), buf, 16);
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendPathCheckSum(uchar myPlayerNum, boost::uint32_t checksum)
{
	PackPacket* packet = new PackPacket(1 + 1 + sizeof(boost::uint32_t), NETMSG_PATH_CHECKSUM);
	*packet << myPlayerNum;
	*packet << checksum;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendCommand(uchar myPlayerNum, int id, uchar options, const std::vector<float>& params)
{
	unsigned size = 9 + params.size() * sizeof(float);
	PackPacket* packet = new PackPacket(size, NETMSG_COMMAND);
	*packet << static_cast<unsigned short>(size) << myPlayerNum << id << options << params;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSelect(uchar myPlayerNum, const std::vector<short>& selectedUnitIDs)
{
	unsigned size = 4 + selectedUnitIDs.size() * sizeof(short);
	PackPacket* packet = new PackPacket(size, NETMSG_SELECT);
	*packet << static_cast<unsigned short>(size) << myPlayerNum << selectedUnitIDs;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendPause(uchar myPlayerNum, uchar bPaused)
{
	PackPacket* packet = new PackPacket(3, NETMSG_PAUSE);
	*packet << myPlayerNum << bPaused;
	return PacketType(packet);
}



PacketType CBaseNetProtocol::SendAICommand(uchar myPlayerNum, short unitID, int id, uchar options, const std::vector<float>& params)
{
	unsigned size = 11 + params.size() * sizeof(float);
	PackPacket* packet = new PackPacket(size, NETMSG_AICOMMAND);
	*packet << static_cast<unsigned short>(size) << myPlayerNum << unitID << id << options << params;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendAIShare(uchar myPlayerNum, uchar sourceTeam, uchar destTeam, float metal, float energy, const std::vector<short>& unitIDs)
{
	boost::uint16_t totalNumBytes = (1 + sizeof(boost::uint16_t)) + (3 + (2 * sizeof(float)) + (unitIDs.size() * sizeof(short)));

	PackPacket* packet = new PackPacket(totalNumBytes, NETMSG_AISHARE);
	*packet << totalNumBytes << myPlayerNum << sourceTeam << destTeam << metal << energy << unitIDs;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendUserSpeed(uchar myPlayerNum, float userSpeed)
{
	PackPacket* packet = new PackPacket(6, NETMSG_USER_SPEED);
	*packet << myPlayerNum << userSpeed;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendInternalSpeed(float internalSpeed)
{
	PackPacket* packet = new PackPacket(5, NETMSG_INTERNAL_SPEED);
	*packet << internalSpeed;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendCPUUsage(float cpuUsage)
{
	PackPacket* packet = new PackPacket(5, NETMSG_CPU_USAGE);
	*packet << cpuUsage;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendCustomData(uchar myPlayerNum, uchar dataType, int dataValue)
{
	PackPacket* packet = new PackPacket(7, NETMSG_CUSTOM_DATA);
	*packet << myPlayerNum << dataType << dataValue;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSpeedControl(uchar myPlayerNum, int speedCtrl) {
	return SendCustomData(myPlayerNum, CUSTOM_DATA_SPEEDCONTROL, speedCtrl);
}

PacketType CBaseNetProtocol::SendDirectControl(uchar myPlayerNum)
{
	PackPacket* packet = new PackPacket(2, NETMSG_DIRECT_CONTROL);
	*packet << myPlayerNum;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendDirectControlUpdate(uchar myPlayerNum, uchar status, short heading, short pitch)
{
	PackPacket* packet = new PackPacket(7, NETMSG_DC_UPDATE);
	*packet << myPlayerNum << status << heading << pitch;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendAttemptConnect(const std::string name, const std::string& passwd, const std::string version, bool reconnect)
{
	boost::uint16_t size = 7 + name.size() + passwd.size() + version.size();
	PackPacket* packet = new PackPacket(size , NETMSG_ATTEMPTCONNECT);
	*packet << size << name << passwd << version << uchar(reconnect);
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendShare(uchar myPlayerNum, uchar shareTeam, uchar bShareUnits, float shareMetal, float shareEnergy)
{
	PackPacket* packet = new PackPacket(12, NETMSG_SHARE);
	*packet << myPlayerNum << shareTeam << bShareUnits << shareMetal << shareEnergy;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSetShare(uchar myPlayerNum, uchar myTeam, float metalShareFraction, float energyShareFraction)
{
	PackPacket* packet = new PackPacket(11, NETMSG_SETSHARE);
	*packet << myPlayerNum << myTeam << metalShareFraction << energyShareFraction;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendSendPlayerStat()
{
	return PacketType(new PackPacket(1, NETMSG_SENDPLAYERSTAT));
}

PacketType CBaseNetProtocol::SendPlayerStat(uchar myPlayerNum, const PlayerStatistics& currentStats)
{
	PackPacket* packet = new PackPacket(2 + sizeof(PlayerStatistics), NETMSG_PLAYERSTAT);
	*packet << myPlayerNum << currentStats;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendGameOver()
{
	return PacketType(new PackPacket(1, NETMSG_GAMEOVER));
}


// [NETMSG_MAPDRAW = 31] uchar messageSize = 9, myPlayerNum, command = MAPDRAW_ERASE; short x, z; bool
PacketType CBaseNetProtocol::SendMapErase(uchar myPlayerNum, short x, short z)
{
	PackPacket* packet = new PackPacket(8, NETMSG_MAPDRAW);
	*packet << static_cast<uchar>(8) << myPlayerNum << static_cast<uchar>(MAPDRAW_ERASE) << x << z;
	return PacketType(packet);
}

// [NETMSG_MAPDRAW = 31] uchar messageSize, uchar myPlayerNum, command = MAPDRAW_POINT; short x, z; bool; std::string label;
PacketType CBaseNetProtocol::SendMapDrawPoint(uchar myPlayerNum, short x, short z, const std::string& label, bool fromLua)
{
	const unsigned size = 9 + label.size() + 1;
	PackPacket* packet = new PackPacket(size, NETMSG_MAPDRAW);
	*packet <<
		static_cast<uchar>(size) <<
		myPlayerNum <<
		static_cast<uchar>(MAPDRAW_POINT) <<
		x <<
		z <<
		uchar(fromLua) <<
		label;
	return PacketType(packet);
}

// [NETMSG_MAPDRAW = 31] uchar messageSize = 13, myPlayerNum, command = MAPDRAW_LINE; short x1, z1, x2, z2; bool
PacketType CBaseNetProtocol::SendMapDrawLine(uchar myPlayerNum, short x1, short z1, short x2, short z2, bool fromLua)
{
	PackPacket* packet = new PackPacket(13, NETMSG_MAPDRAW);
	*packet <<
		static_cast<uchar>(13) <<
		myPlayerNum <<
		static_cast<uchar>(MAPDRAW_LINE) <<
		x1 << z1 <<
		x2 << z2 <<
		uchar(fromLua);
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendSyncResponse(int frameNum, uint checksum)
{
	PackPacket* packet = new PackPacket(9, NETMSG_SYNCRESPONSE);
	*packet << frameNum << checksum;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSystemMessage(uchar myPlayerNum, std::string message)
{
	if (message.size() > 65000)
	{
		message.resize(65000);
		message += "...";
	}
	unsigned size = 1 + 2 + 1 + message.size() + 1;
	PackPacket* packet = new PackPacket(size, NETMSG_SYSTEMMSG);
	*packet << static_cast<boost::uint16_t>(size) << myPlayerNum << message;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendStartPos(uchar myPlayerNum, uchar teamNum, uchar ready, float x, float y, float z)
{
	PackPacket* packet = new PackPacket(16, NETMSG_STARTPOS);
	*packet << myPlayerNum << teamNum << ready << x << y << z;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendPlayerInfo(uchar myPlayerNum, float cpuUsage, int ping)
{
	PackPacket* packet = new PackPacket(10, NETMSG_PLAYERINFO);
	*packet << myPlayerNum << cpuUsage << static_cast<boost::uint32_t>(ping);
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendPlayerLeft(uchar myPlayerNum, uchar bIntended)
{
	PackPacket* packet = new PackPacket(3, NETMSG_PLAYERLEFT);
	*packet << myPlayerNum << bIntended;
	return PacketType(packet);
}

// NETMSG_LUAMSG = 50, uchar myPlayerNum; std::string modName; (e.g. `custom msg')
PacketType CBaseNetProtocol::SendLuaMsg(uchar myPlayerNum, unsigned short script, uchar mode, const std::vector<boost::uint8_t>& msg)
{
	boost::uint16_t size = 7 + msg.size();
	PackPacket* packet = new PackPacket(size, NETMSG_LUAMSG);
	*packet << size << myPlayerNum << script << mode << msg;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendGiveAwayEverything(uchar myPlayerNum, uchar giveToTeam, uchar takeFromTeam)
{
	PackPacket* packet = new PackPacket(5, NETMSG_TEAM);
	*packet << myPlayerNum << static_cast<uchar>(TEAMMSG_GIVEAWAY) << giveToTeam << takeFromTeam;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendResign(uchar myPlayerNum)
{
	PackPacket* packet = new PackPacket(5, NETMSG_TEAM);
	*packet << myPlayerNum << static_cast<uchar>(TEAMMSG_RESIGN) << static_cast<uchar>(0) << static_cast<uchar>(0);
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendJoinTeam(uchar myPlayerNum, uchar wantedTeamNum)
{
	PackPacket* packet = new PackPacket(5, NETMSG_TEAM);
	*packet << myPlayerNum << static_cast<uchar>(TEAMMSG_JOIN_TEAM) << wantedTeamNum << static_cast<uchar>(0);
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendTeamDied(uchar myPlayerNum, uchar whichTeam)
{
	PackPacket* packet = new PackPacket(5, NETMSG_TEAM);
	*packet << myPlayerNum << static_cast<uchar>(TEAMMSG_TEAM_DIED) << whichTeam << static_cast<uchar>(0);
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendAICreated(const uchar myPlayerNum,
                                           const uint  whichSkirmishAI,
                                           const uchar team,
                                           const std::string& name)
{
	// do not hand optimize this math; the compiler will do that
	const uint size = 1 + 1 + 1 + 4 + 1 + (name.size() + 1);
	PackPacket* packet = new PackPacket(size, NETMSG_AI_CREATED);
	*packet
		<< static_cast<uchar>(size)
		<< myPlayerNum
		<< whichSkirmishAI
		<< team
		<< name;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendAIStateChanged(const uchar myPlayerNum,
                                                const uint  whichSkirmishAI,
                                                const uchar newState)
{
	// do not hand optimize this math; the compiler will do that
	PackPacket* packet = new PackPacket(1 + 1 + 4 + 1, NETMSG_AI_STATE_CHANGED);
	*packet << myPlayerNum << whichSkirmishAI << newState;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSetAllied(uchar myPlayerNum, uchar whichAllyTeam, uchar state)
{
	PackPacket* packet = new PackPacket(4, NETMSG_ALLIANCE);
	*packet << myPlayerNum << whichAllyTeam << state;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendRegisterNetMsg( uchar myPlayerNum, NETMSG msgID )
{
	PackPacket* packet = new PackPacket(3, NETMSG_REGISTER_NETMSG);
	*packet << myPlayerNum << (uchar)msgID;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendUnRegisterNetMsg( uchar myPlayerNum, NETMSG msgID )
{
	PackPacket* packet = new PackPacket(3, NETMSG_UNREGISTER_NETMSG);
	*packet << myPlayerNum << (uchar)msgID;
	return PacketType(packet);
}


#ifdef SYNCDEBUG
PacketType CBaseNetProtocol::SendSdCheckrequest(int frameNum)
{
	PackPacket* packet = new PackPacket(5, NETMSG_SD_CHKREQUEST);
	*packet << frameNum;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSdCheckresponse(uchar myPlayerNum, boost::uint64_t flop, std::vector<unsigned> checksums)
{
	unsigned size = 1 + 2 + 1 + 8 + checksums.size() * 4;
	PackPacket* packet = new PackPacket(size, NETMSG_SD_CHKRESPONSE);
	*packet << static_cast<boost::uint16_t>(size) << myPlayerNum << flop << checksums;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSdReset()
{
	return PacketType(new PackPacket(1, NETMSG_SD_RESET));
}

PacketType CBaseNetProtocol::SendSdBlockrequest(unsigned short begin, unsigned short length, unsigned short requestSize)
{
	PackPacket* packet = new PackPacket(7, NETMSG_SD_BLKREQUEST);
	*packet << begin << length << requestSize;
	return PacketType(packet);
	
}

PacketType CBaseNetProtocol::SendSdBlockresponse(uchar myPlayerNum, std::vector<unsigned> checksums)
{
	unsigned size = 1 + 2 + 1 + checksums.size() * 4;
	PackPacket* packet = new PackPacket(size, NETMSG_SD_BLKRESPONSE);
	*packet << static_cast<boost::uint16_t>(size) << myPlayerNum << checksums;
	return PacketType(packet);
}
#endif // SYNCDEBUG
/* FIXME: add these:
 NETMSG_SD_CHKREQUEST    = 41,
 NETMSG_SD_CHKRESPONSE   = 42,
 NETMSG_SD_BLKREQUEST    = 43,
 NETMSG_SD_BLKRESPONSE   = 44,
 NETMSG_SD_RESET         = 45,
*/

CBaseNetProtocol::CBaseNetProtocol()
{
	netcode::ProtocolDef* proto = netcode::ProtocolDef::instance();
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
	proto->AddType(NETMSG_PATH_CHECKSUM, 1 + 1 + sizeof(boost::uint32_t));
	proto->AddType(NETMSG_COMMAND, -2);
	proto->AddType(NETMSG_SELECT, -2);
	proto->AddType(NETMSG_PAUSE, 3);

	proto->AddType(NETMSG_AICOMMAND, -2);
	proto->AddType(NETMSG_AICOMMANDS, -2);
	proto->AddType(NETMSG_AISHARE, -2);

	proto->AddType(NETMSG_USER_SPEED, 6);
	proto->AddType(NETMSG_INTERNAL_SPEED, 5);
	proto->AddType(NETMSG_CPU_USAGE, 5);
	proto->AddType(NETMSG_CUSTOM_DATA, 7);
	proto->AddType(NETMSG_DIRECT_CONTROL, 2);
	proto->AddType(NETMSG_DC_UPDATE, 7);
	proto->AddType(NETMSG_ATTEMPTCONNECT, -2);
	proto->AddType(NETMSG_SHARE, 12);
	proto->AddType(NETMSG_SETSHARE, 11);
	proto->AddType(NETMSG_SENDPLAYERSTAT, 1);
	proto->AddType(NETMSG_PLAYERSTAT, 2 + sizeof(PlayerStatistics));
	proto->AddType(NETMSG_GAMEOVER, 1);
	proto->AddType(NETMSG_MAPDRAW, -1);
	proto->AddType(NETMSG_SYNCRESPONSE, 9);
	proto->AddType(NETMSG_SYSTEMMSG, -2);
	proto->AddType(NETMSG_STARTPOS, 16);
	proto->AddType(NETMSG_PLAYERINFO, 10);
	proto->AddType(NETMSG_PLAYERLEFT, 3);
	proto->AddType(NETMSG_LUAMSG, -2);
	proto->AddType(NETMSG_TEAM, 5);
	proto->AddType(NETMSG_GAMEDATA, -2);
	proto->AddType(NETMSG_ALLIANCE, 4);
	proto->AddType(NETMSG_CCOMMAND, -2);
	proto->AddType(NETMSG_REGISTER_NETMSG, 3 );
	proto->AddType(NETMSG_UNREGISTER_NETMSG, 3);

	proto->AddType(NETMSG_AI_CREATED, -1);
	proto->AddType(NETMSG_AI_STATE_CHANGED, 7);

#ifdef SYNCDEBUG
	proto->AddType(NETMSG_SD_CHKREQUEST, 5);
	proto->AddType(NETMSG_SD_CHKRESPONSE, -2);
	proto->AddType(NETMSG_SD_RESET, 1);
	proto->AddType(NETMSG_SD_BLKREQUEST, 7);
	proto->AddType(NETMSG_SD_BLKRESPONSE, -2);
#endif // SYNCDEBUG
}

CBaseNetProtocol::~CBaseNetProtocol()
{
	//SendQuit();
}
