#include "NetProtocol.h"

#include <SDL_timer.h>

#include "Game/GameSetup.h"
#include "LogOutput.h"
#include "DemoRecorder.h"


CNetProtocol::CNetProtocol()
{
	serverSlot = 0;
	record = 0;
	localDemoPlayback = false;
}

CNetProtocol::~CNetProtocol()
{
	if (record != 0)
		delete record;
}

void CNetProtocol::InitClient(const char *server, unsigned portnum,unsigned sourceport, const unsigned wantedNumber)
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

	serverSlot = myNum;
}

void CNetProtocol::InitLocalClient(const unsigned wantedNumber)
{
	if (!localDemoPlayback)
	{
		record = new CDemoRecorder();
	}

	unsigned myNum = CNet::InitLocalClient(wantedNumber);
	SendAttemptConnect(wantedNumber, NETWORK_VERSION);
	serverSlot = myNum;
}

bool CNetProtocol::IsActiveConnection() const
{
	return CNet::IsActiveConnection(serverSlot);
}

const RawPacket* CNetProtocol::Peek(unsigned ahead) const
{
	return CNet::Peek(serverSlot, ahead);
}

RawPacket* CNetProtocol::GetData()
{
	RawPacket* ret = CNet::GetData(serverSlot);
	
	if (record && ret)
	{
		record->SaveToDemo(ret->data, ret->length);
	}
	
	return ret;
}

void CNetProtocol::UpdateLoop()
{
	loading = true;
	while (loading)
	{
		Update();
		SDL_Delay(1000);
	}
}

CNetProtocol* net=0;
