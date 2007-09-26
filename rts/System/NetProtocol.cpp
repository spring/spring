#include "NetProtocol.h"
#include "Rendering/InMapDraw.h"
#include "Game/GameSetup.h"
#include "LogOutput.h"
#include "DemoRecorder.h"
#include "DemoReader.h"

const unsigned char NETWORK_VERSION = 1;


CNetProtocol::CNetProtocol()
{
	record = 0;
	play = 0;
	localDemoPlayback = false;
	
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
	RegisterMessage(NETMSG_SETSHARE, 10);
	RegisterMessage(NETMSG_SENDPLAYERSTAT, 1);
	RegisterMessage(NETMSG_PLAYERSTAT, 2+sizeof(CPlayer::Statistics));
	RegisterMessage(NETMSG_GAMEOVER, 1);
	RegisterMessage(NETMSG_MAPDRAW, -1);
	RegisterMessage(NETMSG_SYNCREQUEST, 5);
	RegisterMessage(NETMSG_SYNCRESPONSE, 10);
	RegisterMessage(NETMSG_SYSTEMMSG, -1);
	RegisterMessage(NETMSG_STARTPOS, 15);
	RegisterMessage(NETMSG_PLAYERINFO, 10);
	RegisterMessage(NETMSG_PLAYERLEFT, 3);
	RegisterMessage(NETMSG_MODNAME, -1);
}

CNetProtocol::~CNetProtocol()
{
	SendQuit();
	FlushNet();

	if (record != 0)
		delete record;
	if (play != 0)
		delete play;
}

int CNetProtocol::InitServer(const unsigned portnum)
{
	int ret = CNet::InitServer(portnum);
	logOutput.Print("Created server on port %i", portnum);

	imServer = true;
	//TODO demo recording support for server
	return ret;
}

int CNetProtocol::InitServer(const unsigned portnum, const std::string& demoName)
{
	int ret = InitServer(portnum);
	play = new CDemoReader(demoName);
	imServer = true;
	return ret;
}

unsigned CNetProtocol::InitClient(const char *server, unsigned portnum,unsigned sourceport)
{
	unsigned myNum = CNet::InitClient(server, portnum, sourceport,gameSetup ? gameSetup->myPlayer : 0);
	SendAttemptConnect(gameSetup ? gameSetup->myPlayer : 0, NETWORK_VERSION);
	FlushNet();
	imServer = false;

	if (!gameSetup || !gameSetup->hostDemo)	//TODO do we really want this?
	{
		record = new CDemoRecorder();
	}
	
	logOutput.Print("Connected to %s:%i using number %i", server, portnum, gameSetup ? gameSetup->myPlayer : 0);

	return myNum;
}

unsigned CNetProtocol::InitLocalClient(const unsigned wantedNumber)
{
	imServer = false;
	if (!IsDemoServer())
	{
		record = new CDemoRecorder();
		if (!mapName.empty())
			record->SetName(mapName);
	}

	unsigned myNum = CNet::InitLocalClient(wantedNumber);
	SendAttemptConnect(wantedNumber, NETWORK_VERSION);
	//logOutput.Print("Connected to local server using number %i", wantedNumber);
	return myNum;
}

unsigned CNetProtocol::ServerInitLocalClient(const unsigned wantedNumber)
{
	int hisNewNumber = CNet::InitLocalClient(wantedNumber);
	Update();

	if (!IsDemoServer()) {
	// send game data for demo recording
		if (!scriptName.empty())
			SendScript(scriptName);
		if (!mapName.empty())
			SendMapName(mapChecksum, mapName);
		if (!modName.empty())
			SendModName(modChecksum, modName);
	}

	//logOutput.Print("Local client initialised using number %i", hisNewNumber);

	return hisNewNumber;
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
	while (HasIncomingConnection())
	{
		const unsigned inbuflength = 4096;
		unsigned char inbuf[inbuflength];
		
		CNet::Update();
		int ret = CNet::GetData(inbuf);
		
		if (ret >= 3 && inbuf[0] == NETMSG_ATTEMPTCONNECT && inbuf[2] == NETWORK_VERSION)
		{
			unsigned hisNewNumber = AcceptIncomingConnection(inbuf[1]);
			
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
		else
		{
			logOutput.Print("Client AttemptConnect rejected: NETMSG: %i VERSION: %i Length: %i", inbuf[0], inbuf[2], ret);
			RejectIncomingConnection();
		}
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

int CNetProtocol::GetData(unsigned char* buf, const unsigned conNum)
{
	int ret = CNet::GetData(buf, conNum);
	return ret;
}

//  NETMSG_QUIT             = 2,  //

void CNetProtocol::SendQuit()
{
	SendData(NETMSG_QUIT);
}

void CNetProtocol::SendQuit(unsigned playerNum)
{
	unsigned char a = NETMSG_QUIT;
	SendData(&a, 1, playerNum);
}

//  NETMSG_NEWFRAME         = 3,  // int frameNum;

void CNetProtocol::SendNewFrame(int frameNum)
{
	SendData<int>(NETMSG_NEWFRAME, frameNum);
}

//  NETMSG_STARTPLAYING     = 4,  //

void CNetProtocol::SendStartPlaying()
{
	SendData(NETMSG_STARTPLAYING);
}

//  NETMSG_SETPLAYERNUM     = 5,  // uchar myPlayerNum;

void CNetProtocol::SendSetPlayerNum(uchar myPlayerNum)
{
	SendData<uchar>(NETMSG_SETPLAYERNUM, myPlayerNum);
}

//  NETMSG_PLAYERNAME       = 6,  // uchar myPlayerNum; std::string playerName;

void CNetProtocol::SendPlayerName(uchar myPlayerNum, const std::string& playerName)
{
	SendSTLData<uchar, std::string>(NETMSG_PLAYERNAME, myPlayerNum, playerName);
}

//  NETMSG_CHAT             = 7,  // uchar myPlayerNum; std::string message;

void CNetProtocol::SendChat(uchar myPlayerNum, const std::string& message)
{
	SendSTLData<uchar, std::string>(NETMSG_CHAT, myPlayerNum, message);
}

//  NETMSG_RANDSEED         = 8,  // uint randSeed;

void CNetProtocol::SendRandSeed(uint randSeed)
{
	SendData<uint>(NETMSG_RANDSEED, randSeed);
}

//  NETMSG_GAMEID           = 9,  // char gameID[16];

void CNetProtocol::SendGameID(const uchar* buf)
{
	uchar data[17];
	data[0] = NETMSG_GAMEID;
	memcpy(&data[1], buf, 16);
	SendData(data, 17);
}

//  NETMSG_COMMAND          = 11, // uchar myPlayerNum; int id; uchar options; std::vector<float> params;

void CNetProtocol::SendCommand(uchar myPlayerNum, int id, uchar options, const std::vector<float>& params)
{
	SendSTLData<uchar, int, uchar, std::vector<float> >(NETMSG_COMMAND, myPlayerNum, id, options, params);
}

//  NETMSG_SELECT           = 12, // uchar myPlayerNum; std::vector<short> selectedUnitIDs;

void CNetProtocol::SendSelect(uchar myPlayerNum, const std::vector<short>& selectedUnitIDs)
{
	SendSTLData<uchar, std::vector<short> >(NETMSG_SELECT, myPlayerNum, selectedUnitIDs);
}

//  NETMSG_PAUSE            = 13, // uchar playerNum, bPaused;

void CNetProtocol::SendPause(uchar myPlayerNum, uchar bPaused)
{
	SendData<uchar, uchar>(NETMSG_PAUSE, myPlayerNum, bPaused);
}

//  NETMSG_AICOMMAND        = 14, // uchar myPlayerNum; short unitID; int id; uchar options; std::vector<float> params;

void CNetProtocol::SendAICommand(uchar myPlayerNum, short unitID, int id, uchar options, const std::vector<float>& params)
{
	SendSTLData<uchar, short, int, uchar, std::vector<float> >(
			NETMSG_AICOMMAND, myPlayerNum, unitID, id, options, params);
}

//  NETMSG_AICOMMANDS       = 15, // uchar myPlayerNum;
	                              // short unitIDCount;  unitIDCount X short(unitID)
	                              // short commandCount; commandCount X { int id; uchar options; std::vector<float> params }

void CNetProtocol::SendAICommands(uchar myPlayerNum, short unitIDCount, ...)
{
	//FIXME: needs special care; sits in CSelectedUnits::SendCommandsToUnits().
}

//  NETMSG_SCRIPT           = 16, // std::string scriptName;

void CNetProtocol::SendScript(const std::string& newScriptName)
{
	scriptName = newScriptName;
	SendSTLData<std::string> (NETMSG_SCRIPT, scriptName);
}

//  NETMSG_MAPNAME          = 18, // uint checksum; std::string mapName;   (e.g. `SmallDivide.smf')

void CNetProtocol::SendMapName(const uint checksum, const std::string& newMapName)
{
	//logOutput.Print("SendMapName: %s %d", newMapName.c_str(), checksum);
	mapChecksum = checksum;
	mapName = newMapName;
	if (imServer)
		return SendSTLData<uint, std::string>(NETMSG_MAPNAME, checksum, mapName);

	if (record)
		record->SetName(newMapName);
}

//  NETMSG_USER_SPEED       = 19, // uchar myPlayerNum, float userSpeed;
void CNetProtocol::SendUserSpeed(uchar myPlayerNum, float userSpeed)
{
	SendData<uchar, float> (NETMSG_USER_SPEED, myPlayerNum, userSpeed);
}

//  NETMSG_INTERNAL_SPEED   = 20, // float internalSpeed;

void CNetProtocol::SendInternalSpeed(float internalSpeed)
{
	SendData<float> (NETMSG_INTERNAL_SPEED, internalSpeed);
}

//  NETMSG_CPU_USAGE        = 21, // float cpuUsage;

void CNetProtocol::SendCPUUsage(float cpuUsage)
{
	SendData<float> (NETMSG_CPU_USAGE, cpuUsage);
}

//  NETMSG_DIRECT_CONTROL   = 22, // uchar myPlayerNum;

void CNetProtocol::SendDirectControl(uchar myPlayerNum)
{
	SendData<uchar> (NETMSG_DIRECT_CONTROL, myPlayerNum);
}

//  NETMSG_DC_UPDATE        = 23, // uchar myPlayerNum, status; short heading, pitch;

void CNetProtocol::SendDirectControlUpdate(uchar myPlayerNum, uchar status, short heading, short pitch)
{
	SendData<uchar, uchar, short, short> (NETMSG_DC_UPDATE, myPlayerNum, status, heading, pitch);
}

//  NETMSG_ATTEMPTCONNECT   = 25, // uchar myPlayerNum, networkVersion;

void CNetProtocol::SendAttemptConnect(uchar myPlayerNum, uchar networkVersion)
{
	SendData<uchar, uchar> (NETMSG_ATTEMPTCONNECT, myPlayerNum, networkVersion);
}

//  NETMSG_SHARE            = 26, // uchar myPlayerNum, shareTeam, bShareUnits; float shareMetal, shareEnergy;

void CNetProtocol::SendShare(uchar myPlayerNum, uchar shareTeam, uchar bShareUnits, float shareMetal, float shareEnergy)
{
	SendData<uchar, uchar, uchar, float, float> (NETMSG_SHARE, myPlayerNum, shareTeam, bShareUnits, shareMetal, shareEnergy);
}

//  NETMSG_SETSHARE         = 27, // uchar myTeam; float metalShareFraction, energyShareFraction;

void CNetProtocol::SendSetShare(uchar myTeam, float metalShareFraction, float energyShareFraction)
{
	SendData<uchar, float, float>(NETMSG_SETSHARE, myTeam, metalShareFraction, energyShareFraction);
}

//  NETMSG_SENDPLAYERSTAT   = 28, //

void CNetProtocol::SendSendPlayerStat()
{
	SendData(NETMSG_SENDPLAYERSTAT);
}

//  NETMSG_PLAYERSTAT       = 29, // uchar myPlayerNum; CPlayer::Statistics currentStats;

void CNetProtocol::SendPlayerStat(uchar myPlayerNum, const CPlayer::Statistics& currentStats)
{
	SendData<uchar, CPlayer::Statistics>(NETMSG_PLAYERSTAT, myPlayerNum, currentStats);
}

//  NETMSG_GAMEOVER         = 30, //

void CNetProtocol::SendGameOver()
{
	SendData(NETMSG_GAMEOVER);
}

//  NETMSG_MAPDRAW          = 31, // uchar messageSize =  8, myPlayerNum, command = CInMapDraw::NET_ERASE; short x, z;
 // uchar messageSize = 12, myPlayerNum, command = CInMapDraw::NET_LINE; short x1, z1, x2, z2;
 // /*messageSize*/   uchar myPlayerNum, command = CInMapDraw::NET_POINT; short x, z; std::string label;

void CNetProtocol::SendMapErase(uchar myPlayerNum, short x, short z)
{
	SendData<uchar, uchar, uchar, short, short>(NETMSG_MAPDRAW, 8, myPlayerNum, CInMapDraw::NET_ERASE, x, z);
}

void CNetProtocol::SendMapDrawLine(uchar myPlayerNum, short x1, short z1, short x2, short z2)
{
	SendData<uchar, uchar, uchar, short, short, short, short>(NETMSG_MAPDRAW, 12, myPlayerNum, CInMapDraw::NET_LINE, x1, z1, x2, z2);
}

void CNetProtocol::SendMapDrawPoint(uchar myPlayerNum, short x, short z, const std::string& label)
{
	SendSTLData<uchar, uchar, short, short, std::string>(NETMSG_MAPDRAW, myPlayerNum, CInMapDraw::NET_POINT, x, z, label);
}

//  NETMSG_SYNCREQUEST      = 32, // int frameNum;

void CNetProtocol::SendSyncRequest(int frameNum)
{
	SendData<int>(NETMSG_SYNCREQUEST, frameNum);
}

//  NETMSG_SYNCRESPONSE     = 33, // uchar myPlayerNum; int frameNum; uint checksum;

void CNetProtocol::SendSyncResponse(uchar myPlayerNum, int frameNum, uint checksum)
{
	SendData<uchar, int, uint>(NETMSG_SYNCRESPONSE, myPlayerNum, frameNum, checksum);
}

//  NETMSG_SYSTEMMSG        = 35, // uchar myPlayerNum; std::string message;

void CNetProtocol::SendSystemMessage(uchar myPlayerNum, const std::string& message)
{
	SendSTLData<uchar, std::string>(NETMSG_SYSTEMMSG, myPlayerNum, message);
}

//  NETMSG_STARTPOS         = 36, // uchar myTeam, ready /*0: not ready, 1: ready, 2: don't update readiness*/; float x, y, z;

void CNetProtocol::SendStartPos(uchar myTeam, uchar ready, float x, float y, float z)
{
	SendData<uchar, uchar, float, float, float>(NETMSG_STARTPOS, myTeam, ready, x, y, z);
}

//  NETMSG_PLAYERINFO       = 38, // uchar myPlayerNum; float cpuUsage; int ping /*in frames*/;
void CNetProtocol::SendPlayerInfo(uchar myPlayerNum, float cpuUsage, int ping)
{
	SendData<uchar, float, int>(NETMSG_PLAYERINFO, myPlayerNum, cpuUsage, ping);
}

//  NETMSG_PLAYERLEFT       = 39, // uchar myPlayerNum, bIntended /*0: lost connection, 1: left*/;

void CNetProtocol::SendPlayerLeft(uchar myPlayerNum, uchar bIntended)
{
	SendData<uchar, uchar>(NETMSG_PLAYERLEFT, myPlayerNum, bIntended);
}

//  NETMSG_MODNAME          = 40, // uint checksum; std::string modName;   (e.g. `XTA v8.1')

void CNetProtocol::SendModName(const uint checksum, const std::string& newModName)
{
	//logOutput.Print("SendModName: %s %d", newModName.c_str(), checksum);
	modChecksum = checksum;
	modName = newModName;
	if (imServer)
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
