/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <boost/cstdint.hpp>

#include "TUIOHandler.h"
#include "Game/CameraHandler.h"
#include "Game/Camera/CameraController.h"
#include "Game/Camera.h"
#include "System/LogOutput.h"
#include "CommandColors.h"
#include "InputReceiver.h"
#include "GuiHandler.h"
#include "MiniMap.h"
#include "MouseCursor.h"
#include "System/Input/MouseInput.h"
#include "TooltipConsole.h"
#include "Sim/Units/Groups/Group.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/PlayerHandler.h"
#include "Game/UI/UnitTracker.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Lua/LuaInputReceiver.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include "System/myMath.h"
#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern boost::uint8_t *keys;


CTuioHandler* tuio = NULL;

CONFIG(int, TuioPort).defaultValue(3333)
.description("Port number to listen for tuio touch events. Default 3333");

CTuioHandler::CTuioHandler(): activeReceivers(), refreshedReceivers()
{
    int port = configHandler->GetInt("TuioPort");
    client = new SDLTuioClient(port);
    client->addTuioListener(this);
    client->connect();
}

CTuioHandler::~CTuioHandler()
{
    client->disconnect();
    delete client;
}

 void CTuioHandler::addTuioObject(TUIO::TuioObject *tobj)
{
}

void CTuioHandler::updateTuioObject(TUIO::TuioObject *tobj)
{
}

void CTuioHandler::removeTuioObject(TUIO::TuioObject *tobj)
{
}

void CTuioHandler::addTuioCursor(TUIO::TuioCursor *tcur)
{
    std::deque<CInputReceiver*>& inputReceivers = GetInputReceivers();
	std::deque<CInputReceiver*>::iterator ri;

	if (!game->hideInterface) {
		for (ri = inputReceivers.begin(); ri != inputReceivers.end(); ++ri) {
			CInputReceiver* recv=*ri;
			if (recv && recv->addTuioCursor(tcur))
			{
				if (!activeReceivers[tcur->getCursorID()])
					activeReceivers[tcur->getCursorID()] = recv;
				return;
			}
		}
	} else {
		if (luaInputReceiver && luaInputReceiver->addTuioCursor(tcur)) {
            if (!activeReceivers[tcur->getCursorID()])
					activeReceivers[tcur->getCursorID()] = luaInputReceiver;
            return;
		}
		if (guihandler && guihandler->addTuioCursor(tcur)) {
			if (!activeReceivers[tcur->getCursorID()])
					activeReceivers[tcur->getCursorID()] = guihandler;
            return;
		}
	}
}

void CTuioHandler::updateTuioCursor(TUIO::TuioCursor *tcur)
{
    CInputReceiver* recv = activeReceivers[tcur->getCursorID()];
    if(recv)
    {
        recv->updateTuioCursor(tcur);
    }
}

void CTuioHandler::removeTuioCursor(TUIO::TuioCursor *tcur)
{
    CInputReceiver* recv = activeReceivers[tcur->getCursorID()];
    if(recv)
    {
        recv->removeTuioCursor(tcur);
        activeReceivers.erase(tcur->getCursorID());
    }
}

void CTuioHandler::refresh(TUIO::TuioTime ftime)
{
    refreshedReceivers.clear();
    std::map<int, CInputReceiver*>::const_iterator it;


    for(it = activeReceivers.begin(); it != activeReceivers.end(); it++)
    {
        CInputReceiver* recv = it->second;

        if(recv && refreshedReceivers.find(recv) == refreshedReceivers.end())
        {
            recv->tuioRefresh(ftime);
            refreshedReceivers.insert(recv);
        }
    }
}

void CTuioHandler::lock()
{
    client->lockCursorList();
    client->lockObjectList();
}

void CTuioHandler::unlock()
{
    client->unlockCursorList();
    client->unlockObjectList();
}

static inline int min(int val1, int val2) {
	return val1 < val2 ? val1 : val2;
}
static inline int max(int val1, int val2) {
	return val1 > val2 ? val1 : val2;
}

shortint2 toWindowSpace(TUIO::TuioPoint *point)
{
    int nx = point->getScreenX(globalRendering->screenSizeX);
    int ny = point->getScreenY(globalRendering->screenSizeY);

    nx -= globalRendering->winPosX;
    ny -= globalRendering->winPosY;

    shortint2 pnt;
    pnt.x = nx;
    pnt.y = ny;

    return pnt;
}

void clampToWindowSpace(shortint2 &pnt)
{
    pnt.x = max(0, (int) pnt.x);
    pnt.x = min(globalRendering->viewSizeX, (int) pnt.x);

    pnt.y = max(0, (int) pnt.y);
    pnt.y = min(globalRendering->viewSizeY, (int )pnt.y);
}

bool isInWindowSpace(const shortint2 &pnt)
{
    return pnt.x > 0 && pnt.y > 0 && pnt.x < globalRendering->viewSizeX && pnt.y < globalRendering->viewSizeY;
}
