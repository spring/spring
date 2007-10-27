#include "NetProtocol.h"
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
	}

	unsigned myNum = CNet::InitLocalClient(wantedNumber);
	Listening(false);
	SendAttemptConnect(wantedNumber, NETWORK_VERSION);
	//logOutput.Print("Connected to local server using number %i", wantedNumber);
	return myNum;
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

CNetProtocol* net=0;
