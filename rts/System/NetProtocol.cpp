#include "StdAfx.h"
#include "NetProtocol.h"
#include "Rendering/InMapDraw.h"
#include "Game/GameSetup.h"
#include "Game/Team.h"
#include "LogOutput.h"

#define NETWORK_VERSION 1


CNetProtocol::CNetProtocol()
{
	record = 0;
	play = 0;
	localDemoPlayback = false;
	fragbufLength = 0;
	memset(fragbuf, 0, sizeof(fragbuf));
}

CNetProtocol::~CNetProtocol()
{
	if(connected)
	{
		SendQuit();
		FlushNet();
	}

	if (record != 0)
		delete record;
	if (play != 0)
		delete play;
}

int CNetProtocol::InitServer(const unsigned portnum)
{
	int ret = CNet::InitServer(portnum);
	logOutput.Print("Created server on port %i", portnum);

	//TODO demo recording support for server
	return ret;
}

int CNetProtocol::InitServer(const unsigned portnum, const std::string& demoName)
{
	int ret = InitServer(portnum);
	play = new CDemoReader(demoName);
	return ret;
}

int CNetProtocol::InitClient(const char *server, unsigned portnum,unsigned sourceport)
{
	int error = CNet::InitClient(server, portnum, sourceport,gameSetup ? gameSetup->myPlayer : 0);
	SendAttemptConnect(gameSetup ? gameSetup->myPlayer : 0, NETWORK_VERSION);
	FlushNet();
	inInitialConnect=true;

	if (!gameSetup || !gameSetup->hostDemo)	//TODO do we really want this?
	{
		record = new CDemoRecorder();
	}
	if (error == 1)
		logOutput.Print("Connected to %s:%i using number %i", server, portnum, gameSetup ? gameSetup->myPlayer : 0);

	return error;
}

int CNetProtocol::InitLocalClient(const unsigned wantedNumber)
{
	if (!IsDemoServer())
	{
		record = new CDemoRecorder();
		if (!mapName.empty())
			record->SetName(mapName);
	}

	int error = CNet::InitLocalClient(wantedNumber);
	SendAttemptConnect(wantedNumber, NETWORK_VERSION);
	logOutput.Print("Created local client with number %i", wantedNumber);
	return error;
}

int CNetProtocol::ServerInitLocalClient(const unsigned wantedNumber)
{
	Pending buffer;
	buffer.wantedNumber = wantedNumber;
	int hisNewNumber = InitNewConn(buffer, true);
	gs->players[hisNewNumber]->active=true;

	if (!IsDemoServer()) {
	// send game data for demo recording
		if (!scriptName.empty())
			SendScript(scriptName);
		if (!mapName.empty())
			SendMapName(mapChecksum, mapName);
		if (!modName.empty())
			SendModName(modChecksum, modName);
	}

	logOutput.Print("Listening to local client on connection %i", hisNewNumber);

	return 1;
}

void CNetProtocol::Update()
{
	// when hosting a demo, read from file and broadcast data
	if (play != 0 && imServer) {
		unsigned char demobuffer[netcode::NETWORK_BUFFER_SIZE];
		unsigned length = play->GetData(demobuffer, netcode::NETWORK_BUFFER_SIZE);
		if (length > 0) {
			RawSend(demobuffer, length);
		}
	}

	// call our CNet function
	CNet::Update();

	// handle new connections
	while (!justConnected.empty())
	{
		Pending it=justConnected.back();
		justConnected.pop_back();
		if (it.netmsg != NETMSG_ATTEMPTCONNECT || it.networkVersion != NETWORK_VERSION)
		{
			logOutput.Print("Client AttemptConnect rejected: NETMSG: %i VERSION: %i", it.netmsg, it.networkVersion);
			continue;
		}
		unsigned hisNewNumber = InitNewConn(it);

		if(imServer){	// send server data if already decided
			SendSetPlayerNum(hisNewNumber);

			if (!scriptName.empty())
				SendScript(scriptName);
			if (!mapName.empty())
				SendMapName(mapChecksum, mapName);
			if (!modName.empty())
				SendModName(modChecksum, modName);

			for(unsigned a=0;a<gs->activePlayers;a++){
				if(!gs->players[a]->readyToStart)
					continue;
				SendPlayerName(a, gs->players[a]->playerName);
			}
			if(gameSetup){
				for(unsigned a=0;a<gs->activeTeams;a++){
					SendStartPos(a, 2, gs->Team(a)->startPos.x,
								 gs->Team(a)->startPos.y, gs->Team(a)->startPos.z);
				}
			}
			FlushNet();
		}
		gs->players[hisNewNumber]->active=true;
	}
}

bool CNetProtocol::IsDemoServer() const
{
	return (play != NULL || localDemoPlayback);
}

void CNetProtocol::RawSend(const uchar* data,const unsigned length)
{
	SendData(data, length);
}

int CNetProtocol::GetData(unsigned char* buf, const unsigned length, const unsigned conNum, int* que)
{
	// If we got the start of a fragmented message left,
	// we insert it at the start of the buffer.
	if (fragbufLength != 0) {
		if (fragbufLength > length) {
			logOutput.Print("Overflow in incoming network buffer");
			return 0; // just return we read 0 bytes, it isn't too severe after all...
		}
		memcpy(buf, fragbuf, fragbufLength);
	}

	int ret = CNet::GetData(buf + fragbufLength, length - fragbufLength, conNum);

	if (ret <= 0) // error in CNet::GetData or 0 bytes read so nothing to do..
		return ret;
	ret += fragbufLength;

	int readahead = ReadAhead(buf, ret, que);

	// If we got a fragmented packet on end, store it for a later call to GetData.
	fragbufLength = ret - readahead;
	if (fragbufLength != 0)
		memcpy(fragbuf, buf + readahead, fragbufLength);

	// We only return complete messages here (so use readahead instead of ret).
	if (record != NULL && !imServer)
		record->SaveToDemo(buf, readahead);
	return readahead;
}
// 	NETMSG_HELLO            = 1,  //

void CNetProtocol::SendHello()
{
	PingAll();
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

int CNetProtocol::SendScript(const std::string& newScriptName)
{
	scriptName = newScriptName;
	return SendSTLData<std::string> (NETMSG_SCRIPT, scriptName);
}

//  NETMSG_MAPNAME          = 18, // uint checksum; std::string mapName;   (e.g. `SmallDivide.smf')

int CNetProtocol::SendMapName(const uint checksum, const std::string& newMapName)
{
	//logOutput.Print("SendMapName: %s %d", newMapName.c_str(), checksum);
	mapChecksum = checksum;
	mapName = newMapName;
	if (imServer)
		return SendSTLData<uint, std::string>(NETMSG_MAPNAME, checksum, mapName);

	if (record)
		record->SetName(newMapName);

	return 0;
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

int CNetProtocol::SendModName(const uint checksum, const std::string& newModName)
{
	//logOutput.Print("SendModName: %s %d", newModName.c_str(), checksum);
	modChecksum = checksum;
	modName = newModName;
	if (imServer)
		return SendSTLData<uint, std::string> (NETMSG_MODNAME, checksum, modName);
	return 0;
}

/** @brief Return the length (in bytes) of the first net message in inbuf.
If the message doesn't fit in inbuf, it returns 0, so it can also be used to
check whether a net buffer contains a complete message. */
int CNetProtocol::GetMessageLength(const unsigned char* inbuf, int inbuflength) const
{
	if (inbuflength <= 0)
		return 0;

	int length = 0;
	switch (inbuf[0]) {
		case NETMSG_HELLO:
		case NETMSG_QUIT:
		case NETMSG_STARTPLAYING:
		case NETMSG_MEMDUMP:
		case NETMSG_SENDPLAYERSTAT:
		case NETMSG_GAMEOVER:
			length = 1;
			break;
		case NETMSG_SETPLAYERNUM:
#ifdef DIRECT_CONTROL_ALLOWED
		case NETMSG_DIRECT_CONTROL:
#endif
			length = 2;
			break;
		case NETMSG_PAUSE:
		case NETMSG_ATTEMPTCONNECT:
		case NETMSG_PLAYERLEFT:
			length = 3;
			break;
		case NETMSG_NEWFRAME:
		case NETMSG_RANDSEED:
		case NETMSG_INTERNAL_SPEED:
		case NETMSG_USER_SPEED:
		case NETMSG_CPU_USAGE:
		case NETMSG_SYNCREQUEST:
			length = 5;
			break;
#ifdef DIRECT_CONTROL_ALLOWED
		case NETMSG_DC_UPDATE:
			length = 7;
			break;
#endif
		case NETMSG_SETSHARE:
		case NETMSG_PLAYERINFO:
		case NETMSG_SYNCRESPONSE:
			length = 10;
			break;
		case NETMSG_SHARE:
			length = 12;
			break;
		case NETMSG_STARTPOS:
			length = 15;
			break;
		case NETMSG_PLAYERSTAT:
			length = sizeof(CPlayer::Statistics) + 2;
			break;
		case NETMSG_PLAYERNAME:
		case NETMSG_CHAT:
		case NETMSG_SYSTEMMSG:
		case NETMSG_MAPNAME:
		case NETMSG_MODNAME:
		case NETMSG_SCRIPT:
		case NETMSG_MAPDRAW:
			if (2 > inbuflength) // there was no room for the size field
				return 0;
			length = inbuf[1];
			break;
		case NETMSG_COMMAND:
		case NETMSG_SELECT:
		case NETMSG_AICOMMAND:
		case NETMSG_AICOMMANDS:
			if (3 > inbuflength) // there was no room for the size field
				return 0;
			length = *((const short int*)&inbuf[1]);
			break;
		default:{
#ifdef SYNCDEBUG
			// maybe something for the sync debugger?
			// FIXME fix sync debugger so it handles truncated messages correctly
			length = CSyncDebugger::GetInstance()->GetMessageLength(inbuf);
			if (length == 0)
#endif
			{
				logOutput.Print("Unknown net msg in read ahead %i", inbuf[0]);
				length = 1;
			}
			break;
		}
	}

	if (length > inbuflength)
		return 0;

	return length;
}

/** @brief This performs a read ahead in the network buffer.
It returns the position in inbuf on which the first fragmented message starts.
If que is not null it is incremented for each NETMSG_NEWFRAME message in the
buffer. */
int CNetProtocol::ReadAhead(const unsigned char* inbuf, int inbuflength, int* que) const
{
	int pos = 0;
	int lastLength;

	while ((lastLength = GetMessageLength(inbuf + pos, inbuflength - pos)) != 0) {
		if (inbuf[pos] == NETMSG_NEWFRAME && que != NULL)
			++*que;
		pos += lastLength;
	}

	return pos;
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
