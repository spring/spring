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
	
	logOutput.Print("Connected to %s:%i using number %i", server, portnum, myNum);

	return myNum;
}

unsigned CNetProtocol::InitLocalClient(const unsigned wantedNumber)
{
	if (!localDemoPlayback)
	{
		record = new CDemoRecorder();
	}

	unsigned myNum = CNet::InitLocalClient(wantedNumber);
	SendAttemptConnect(wantedNumber, NETWORK_VERSION);
	return myNum;
}

RawPacket* CNetProtocol::GetData(const unsigned conNum)
{
	RawPacket* ret = CBaseNetProtocol::GetData(conNum);
	
	if (record && ret)
	{
		record->SaveToDemo(ret->data, ret->length);
	}
	
	return ret;
}

CNetProtocol* net=0;
