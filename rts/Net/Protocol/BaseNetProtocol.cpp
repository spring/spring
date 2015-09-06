/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BaseNetProtocol.h"

#include "Game/Players/PlayerStatistics.h"
#include "Sim/Misc/TeamStatistics.h"
#include "System/Net/RawPacket.h"
#include "System/Net/PackPacket.h"
#include "System/Net/ProtocolDef.h"
#include <boost/cstdint.hpp>

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



PacketType CBaseNetProtocol::SendAICommand(uchar myPlayerNum, unsigned char aiID, short unitID, int id, int aiCommandId, uchar options, const std::vector<float>& params)
{
	int cmdTypeId = NETMSG_AICOMMAND;
	unsigned size = 12 + (params.size() * sizeof(float));
	if (aiCommandId != -1) {
		cmdTypeId = NETMSG_AICOMMAND_TRACKED;
		size += 4;
	}
	PackPacket* packet = new PackPacket(size, cmdTypeId);
	*packet << static_cast<unsigned short>(size) << myPlayerNum << aiID << unitID << id << options;
	if (cmdTypeId == NETMSG_AICOMMAND_TRACKED) {
		*packet << aiCommandId;
	}
	*packet << params;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendAIShare(uchar myPlayerNum, unsigned char aiID, uchar sourceTeam, uchar destTeam, float metal, float energy, const std::vector<short>& unitIDs)
{
	boost::uint16_t totalNumBytes = 1 + sizeof(boost::uint16_t) + 1 + 1 + 1 + 1 + (2 * sizeof(float)) + (unitIDs.size() * sizeof(short));

	PackPacket* packet = new PackPacket(totalNumBytes, NETMSG_AISHARE);
	*packet << totalNumBytes << myPlayerNum << aiID << sourceTeam << destTeam << metal << energy << unitIDs;
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


PacketType CBaseNetProtocol::SendAttemptConnect(const std::string& name, const std::string& passwd, const std::string& version, int netloss, bool reconnect)
{
	boost::uint16_t size = 10 + name.size() + passwd.size() + version.size();
	PackPacket* packet = new PackPacket(size , NETMSG_ATTEMPTCONNECT);
	*packet << size << NETWORK_VERSION << name << passwd << version << uchar(reconnect) << uchar(netloss);
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendRejectConnect(const std::string& reason)
{
	unsigned size = 3 + reason.size() + 1;
	PackPacket* packet = new PackPacket(size, NETMSG_REJECT_CONNECT);
	*packet << static_cast<boost::uint16_t>(size) << reason;
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


PacketType CBaseNetProtocol::SendPlayerStat(uchar myPlayerNum, const PlayerStatistics& currentStats)
{
	PackPacket* packet = new PackPacket(2 + sizeof(PlayerStatistics), NETMSG_PLAYERSTAT);
	*packet << myPlayerNum << currentStats;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendTeamStat(uchar teamNum, const TeamStatistics& currentStats)
{
	PackPacket* packet = new netcode::PackPacket(2 + sizeof(TeamStatistics), NETMSG_TEAMSTAT);
	*packet << teamNum << currentStats;
	return PacketType(packet);
}



PacketType CBaseNetProtocol::SendGameOver(uchar myPlayerNum, const std::vector<uchar>& winningAllyTeams)
{
	const unsigned size = (3 * sizeof(uchar)) + (winningAllyTeams.size() * sizeof(uchar));
	PackPacket* packet = new PackPacket(size, NETMSG_GAMEOVER);
	*packet << static_cast<uchar>(size) << myPlayerNum << winningAllyTeams;
	return PacketType(packet);
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


PacketType CBaseNetProtocol::SendSyncResponse(uchar myPlayerNum, int frameNum, uint checksum)
{
	PackPacket* packet = new PackPacket(10, NETMSG_SYNCRESPONSE);
	*packet << myPlayerNum << frameNum << checksum;
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

PacketType CBaseNetProtocol::SendStartPos(uchar myPlayerNum, uchar teamNum, uchar readyState, float x, float y, float z)
{
	PackPacket* packet = new PackPacket(16, NETMSG_STARTPOS);
	*packet << myPlayerNum << teamNum << readyState << x << y << z;
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
	if ((7 + msg.size()) >= (1 << (sizeof(boost::uint16_t) * 8)))
		throw netcode::PackPacketException("Maximum size exceeded");
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
                                           const uchar whichSkirmishAI,
                                           const uchar team,
                                           const std::string& name)
{
	// do not hand optimize this math; the compiler will do that
	const uint size = 1 + 1 + 1 + 1 + 1 + (name.size() + 1);
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
                                                const uchar whichSkirmishAI,
                                                const uchar newState)
{
	// do not hand optimize this math; the compiler will do that
	PackPacket* packet = new PackPacket(1 + 1 + 1 + 1, NETMSG_AI_STATE_CHANGED);
	*packet << myPlayerNum << whichSkirmishAI << newState;
	return PacketType(packet);
}

PacketType CBaseNetProtocol::SendSetAllied(uchar myPlayerNum, uchar whichAllyTeam, uchar state)
{
	PackPacket* packet = new PackPacket(4, NETMSG_ALLIANCE);
	*packet << myPlayerNum << whichAllyTeam << state;
	return PacketType(packet);
}


PacketType CBaseNetProtocol::SendCreateNewPlayer( uchar playerNum, bool spectator, uchar teamNum, std::string playerName )
{
	unsigned size = 1 + sizeof(uchar) + sizeof(uchar) + sizeof(uchar) + sizeof (boost::uint16_t) +playerName.size()+1;
	PackPacket* packet = new PackPacket( size, NETMSG_CREATE_NEWPLAYER);
	*packet << static_cast<boost::uint16_t>(size) << playerNum << (uchar)spectator << teamNum << playerName;
	return PacketType(packet);

}

PacketType CBaseNetProtocol::SendCurrentFrameProgress(int frameNum)
{
	PackPacket* packet = new PackPacket(5, NETMSG_GAME_FRAME_PROGRESS);
	*packet << frameNum;
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


CBaseNetProtocol::CBaseNetProtocol()
{
	netcode::ProtocolDef* proto = netcode::ProtocolDef::GetInstance();
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
	proto->AddType(NETMSG_AICOMMAND_TRACKED, -2);
	proto->AddType(NETMSG_AICOMMANDS, -2);
	proto->AddType(NETMSG_AISHARE, -2);

	proto->AddType(NETMSG_USER_SPEED, 6);
	proto->AddType(NETMSG_INTERNAL_SPEED, 5);
	proto->AddType(NETMSG_CPU_USAGE, 5);
	proto->AddType(NETMSG_DIRECT_CONTROL, 2);
	proto->AddType(NETMSG_DC_UPDATE, 7);
	proto->AddType(NETMSG_ATTEMPTCONNECT, -2);
	proto->AddType(NETMSG_REJECT_CONNECT, -2);
	proto->AddType(NETMSG_SHARE, 12);
	proto->AddType(NETMSG_SETSHARE, 11);

	proto->AddType(NETMSG_PLAYERSTAT, 2 + sizeof(PlayerStatistics));
	proto->AddType(NETMSG_GAMEOVER, -1);
	proto->AddType(NETMSG_MAPDRAW, -1);
	proto->AddType(NETMSG_SYNCRESPONSE, 10);
	proto->AddType(NETMSG_SYSTEMMSG, -2);
	proto->AddType(NETMSG_STARTPOS, 16);
	proto->AddType(NETMSG_PLAYERINFO, 10);
	proto->AddType(NETMSG_PLAYERLEFT, 3);
	proto->AddType(NETMSG_LUAMSG, -2);
	proto->AddType(NETMSG_TEAM, 5);
	proto->AddType(NETMSG_GAMEDATA, -2);
	proto->AddType(NETMSG_ALLIANCE, 4);
	proto->AddType(NETMSG_CCOMMAND, -2);
	proto->AddType(NETMSG_TEAMSTAT, 2 + sizeof(TeamStatistics));
	proto->AddType(NETMSG_REQUEST_TEAMSTAT, 4 );

	proto->AddType(NETMSG_CREATE_NEWPLAYER, -2);

	proto->AddType(NETMSG_AI_CREATED, -1);
	proto->AddType(NETMSG_AI_STATE_CHANGED, 4);
	proto->AddType(NETMSG_GAME_FRAME_PROGRESS,5);

#ifdef SYNCDEBUG
	proto->AddType(NETMSG_SD_CHKREQUEST, 5);
	proto->AddType(NETMSG_SD_CHKRESPONSE, -2);
	proto->AddType(NETMSG_SD_RESET, 1);
	proto->AddType(NETMSG_SD_BLKREQUEST, 7);
	proto->AddType(NETMSG_SD_BLKRESPONSE, -2);
#endif // SYNCDEBUG
}

