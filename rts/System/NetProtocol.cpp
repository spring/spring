#include "NetProtocol.h"
#include "Rendering/InMapDraw.h"
#include "Game/GameSetup.h"
#include "LogOutput.h"
#include "DemoRecorder.h"


CNetProtocol::CNetProtocol()
{
	record = 0;
	localDemoPlayback = false;
}

CNetProtocol::~CNetProtocol()
{
	if (record != 0)
		delete record;
}

int CNetProtocol::InitServer(const unsigned portnum)
{
	CNet::InitServer(portnum);
	logOutput.Print("Created server on port %i", portnum);

	//TODO demo recording support for server
	return 0;
}

unsigned CNetProtocol::InitClient(const char *server, unsigned portnum,unsigned sourceport, const unsigned wantedNumber)
{
	unsigned myNum = CNet::InitClient(server, portnum, sourceport,wantedNumber);
	Listening(false);
	SendAttemptConnect(wantedNumber, NETWORK_VERSION);
	FlushNet();

	if (!gameSetup || !gameSetup->hostDemo)	//TODO do we really want this?
	{
		record = new CDemoRecorder();
	}
	
	logOutput.Print("Connected to %s:%i using number %i", server, portnum, wantedNumber);

	return myNum;
}

unsigned CNetProtocol::InitLocalClient(const unsigned wantedNumber)
{
	if (!IsDemoServer())
	{
		record = new CDemoRecorder();
		if (!mapName.empty())
			record->SetName(mapName);
	}

	unsigned myNum = CNet::InitLocalClient(wantedNumber);
	Listening(false);
	SendAttemptConnect(wantedNumber, NETWORK_VERSION);
	//logOutput.Print("Connected to local server using number %i", wantedNumber);
	return myNum;
}

unsigned CNetProtocol::ServerInitLocalClient(const unsigned wantedNumber)
{
	int hisNewNumber = CNet::InitLocalClient(wantedNumber);
	Update();

	// send game data for demo recording
	if (!scriptName.empty())
		SendScript(scriptName);
	if (!mapName.empty())
		SendMapName(mapChecksum, mapName);
	if (!modName.empty())
		SendModName(modChecksum, modName);

	//logOutput.Print("Local client initialised using number %i", hisNewNumber);

	return hisNewNumber;
}

void CNetProtocol::Update()
{
	// call our CNet function
	CNet::Update();
	// handle new connections
	while (HasIncomingConnection())
	{
		const unsigned inbuflength = 4096;
		unsigned char inbuf[inbuflength];
		int ret = CNet::GetData(inbuf);
		
		if (ret >= 3 && inbuf[0] == NETMSG_ATTEMPTCONNECT && inbuf[2] == NETWORK_VERSION)
		{
			unsigned hisNewNumber = AcceptIncomingConnection(inbuf[1]);
			
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
	return (localDemoPlayback);
}

int CNetProtocol::GetData(unsigned char* buf, const unsigned conNum)
{
	int ret = CBaseNetProtocol::GetData(buf, conNum);
	
	if (record && ret > 0)
	{
		record->SaveToDemo(buf, ret);
	}
	
	return ret;
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
	return SendSTLData<uint, std::string>(NETMSG_MAPNAME, checksum, mapName);

	if (record)
		record->SetName(newMapName);
}

//  NETMSG_MODNAME          = 40, // uint checksum; std::string modName;   (e.g. `XTA v8.1')

void CNetProtocol::SendModName(const uint checksum, const std::string& newModName)
{
	//logOutput.Print("SendModName: %s %d", newModName.c_str(), checksum);
	modChecksum = checksum;
	modName = newModName;
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
