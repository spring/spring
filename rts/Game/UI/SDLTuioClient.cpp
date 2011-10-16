/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <algorithm>
#include <boost/cstdint.hpp>

#include "SDLTuioClient.h"
#include "System/LogOutput.h"
#include "InputReceiver.h"
#include "System/Input/InputHandler.h"
#include "TooltipConsole.h"
#include "Sim/Units/Groups/Group.h"
#include "Lua/LuaInputReceiver.h"

#include "System/mmgr.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include <boost/bind.hpp>
#include "lib/oscpack/OscReceivedElements.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SDLTuioClient::SDLTuioClient(int port):TuioClient(port)
{
    this->port = port;
    input.AddHandler(boost::bind(&SDLTuioClient::ProcessPacketEvents, this, _1));
}


bool SDLTuioClient::ProcessPacketEvents(const SDL_Event& event)
{
    if(event.type == SDL_TUIOEVENT)
    {
        if(event.user.data2 == (void*)this)
        {
            osc::ReceivedPacket *p = (osc::ReceivedPacket*) (event.user.data1);

            try
            {
                if(p->IsBundle())
                    this->ProcessBundle( osc::ReceivedBundle(*p));
                else
                    this->ProcessMessage( osc::ReceivedMessage(*p));
            }
            catch (osc::MalformedBundleException& e)
            {
                std::cerr << "malformed OSC bundle: " << e.what() << std::endl;
            }

            free((void*)(const_cast< char * >(p->Contents())));
            delete p;
        }
    }
    return false;
}

void SDLTuioClient::ProcessPacket( const char *data, int size, const IpEndpointName& remoteEndpoint )
{
    SDL_Event packetEvent;
    packetEvent.user.code = port;

	try
	{
        char *copiedData = (char*) malloc(size);
        memcpy(copiedData,data,size);
        osc::ReceivedPacket *p = new osc::ReceivedPacket( copiedData, size );

        packetEvent.user.data1 = (void*)p;
        packetEvent.user.data2 = (void*)this;

        packetEvent.user.type = SDL_TUIOEVENT;

        SDL_PushEvent(&packetEvent);
	}
	catch (osc::MalformedBundleException& e)
	{
		std::cerr << "malformed OSC bundle: " << e.what() << std::endl;
	}
}
