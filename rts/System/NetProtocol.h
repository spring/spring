#ifndef NETPROTOCOL_H
#define NETPROTOCOL_H

#include "Net.h"
#include "Game/Player.h"

/** High level network code layer */
class CNetProtocol : public CNet {
public:
	typedef unsigned char uchar;
	typedef unsigned int uint;
	int SendHello();
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
	int SendMapName(uint checksum, std::string mapName);
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
	int SendModName(uint checksum, std::string modName);
};

extern CNetProtocol* serverNet;
extern CNetProtocol* net;

#endif
