#include "Connection.h"

#include <SDL_timer.h>

namespace netcode {

CConnection::CConnection()
{
	dataSent = 0; sentOverhead = 0;
	dataRecv = 0; recvOverhead = 0;
	active=true;
	lastReceiveTime= static_cast<float>(SDL_GetTicks())/1000.0f;
	lastSendTime=0;
}

CConnection::~CConnection()
{
}

} // namespace netcode
