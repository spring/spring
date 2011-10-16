/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SDLTUIOCLIENT_H
#define SDLTUIOCLIENT_H

#include <string>
#include <vector>
#include <map>
#include <hash_map>
#include <hash_set>
#include "System/Input/InputHandler.h"

#include "lib/tuio/TuioClient.h"
#include "lib/tuio/TuioObject.h"
#include "System/myMath.h"

#define SDL_TUIOEVENT SDL_USEREVENT + 1

class SDLTuioClient : public TUIO::TuioClient
{

public:
	SDLTuioClient(int port = 3333);
	bool ProcessPacketEvents(const SDL_Event& event);

protected:
    void ProcessPacket( const char *data, int size, const IpEndpointName& remoteEndpoint);


private:
    int port;
};

//extern CTuioHandler* tuio;

#endif /* SDLTUIOCLIENT_H */
