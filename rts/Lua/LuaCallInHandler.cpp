#include "StdAfx.h"
// LuaCallInHandler.cpp: implementation of the CLuaCallInHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaCallInHandler.h"

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "LuaHandle.h"
#include "LuaOpenGL.h"


CLuaCallInHandler luaCallIns;


/******************************************************************************/
/******************************************************************************/

CLuaCallInHandler::CLuaCallInHandler()
{
}


CLuaCallInHandler::~CLuaCallInHandler()
{
}


/******************************************************************************/
/******************************************************************************/

void CLuaCallInHandler::AddHandle(CLuaHandle* lh)
{
	ListInsert(handles, lh);

#define ADDHANDLE(name) \
  if (lh->HasCallIn(#name)) { ListInsert(list ## name, lh); }
  
	ADDHANDLE(Update);

	ADDHANDLE(GameOver);
	ADDHANDLE(TeamDied);

	ADDHANDLE(UnitCreated);
	ADDHANDLE(UnitFinished);
	ADDHANDLE(UnitFromFactory);
	ADDHANDLE(UnitDestroyed);
	ADDHANDLE(UnitTaken);
	ADDHANDLE(UnitGiven);

	ADDHANDLE(UnitIdle);
	ADDHANDLE(UnitDamaged);

	ADDHANDLE(UnitSeismicPing);
	ADDHANDLE(UnitEnteredRadar);
	ADDHANDLE(UnitEnteredLos);
	ADDHANDLE(UnitLeftRadar);
	ADDHANDLE(UnitLeftLos);

	ADDHANDLE(UnitLoaded);
	ADDHANDLE(UnitUnloaded);

	ADDHANDLE(FeatureCreated);
	ADDHANDLE(FeatureDestroyed);

	ADDHANDLE(DrawWorld);
	ADDHANDLE(DrawWorldShadow);
	ADDHANDLE(DrawWorldReflection);
	ADDHANDLE(DrawWorldRefraction);
	ADDHANDLE(DrawScreen);
	ADDHANDLE(DrawInMiniMap);
}


void CLuaCallInHandler::RemoveHandle(CLuaHandle* lh)
{
	ListRemove(handles, lh);

	ListRemove(listUpdate, lh);

	ListRemove(listGameOver, lh);
	ListRemove(listTeamDied, lh);

	ListRemove(listUnitCreated, lh);
	ListRemove(listUnitFinished, lh);
	ListRemove(listUnitFromFactory, lh);
	ListRemove(listUnitDestroyed, lh);

	ListRemove(listUnitTaken, lh);
	ListRemove(listUnitGiven, lh);

	ListRemove(listUnitIdle, lh);
	ListRemove(listUnitDamaged, lh);

	ListRemove(listUnitSeismicPing, lh);
	ListRemove(listUnitEnteredRadar, lh);
	ListRemove(listUnitEnteredLos, lh);
	ListRemove(listUnitLeftRadar, lh);
	ListRemove(listUnitLeftLos, lh);

	ListRemove(listUnitLoaded, lh);
	ListRemove(listUnitUnloaded, lh);

	ListRemove(listFeatureCreated, lh);
	ListRemove(listFeatureDestroyed, lh);

	ListRemove(listDrawWorld, lh);
	ListRemove(listDrawWorldShadow, lh);
	ListRemove(listDrawWorldReflection, lh);
	ListRemove(listDrawWorldRefraction, lh);
	ListRemove(listDrawScreen, lh);
	ListRemove(listDrawInMiniMap, lh);
}


/******************************************************************************/

void CLuaCallInHandler::ListInsert(CallInList& ciList, CLuaHandle* lh)
{
	CallInList::iterator it;
	for (it = ciList.begin(); it != ciList.end(); ++it) {
		const CLuaHandle* lhList = *it;
		if (lh == lhList) {
			return; // already in the list
		}
		else if ((lh->order < lhList->order) ||
		         ((lh->order == lhList->order) &&
		          (lh->name < lhList->name))) {
			ciList.insert(it, lh);
			return;
		}
	}
	ciList.push_back(lh);
}


void CLuaCallInHandler::ListRemove(CallInList& ciList, CLuaHandle* lh)
{
	CallInList newList;
	for (int i = 0; i < ciList.size(); i++) {
		if (lh != ciList[i]) {
			newList.push_back(ciList[i]);
		}
	}
	ciList = newList;
}
		

/******************************************************************************/
/******************************************************************************/

void CLuaCallInHandler::Update()
{
    const int count = listUpdate.size();
    for (int i = 0; i < count; i++) {
      CLuaHandle* lh = listUpdate[i];
      lh->Update();
    }
}


void CLuaCallInHandler::GameOver()
{
    const int count = listGameOver.size();
    for (int i = 0; i < count; i++) {
      CLuaHandle* lh = listGameOver[i];
      lh->GameOver();
    }
}


void CLuaCallInHandler::TeamDied(int teamID)
{
    const int count = listTeamDied.size();
    for (int i = 0; i < count; i++) {
      CLuaHandle* lh = listTeamDied[i];
      lh->TeamDied(teamID);
    }
}


/******************************************************************************/

#define DRAW_CALLIN(name)                         \
  void CLuaCallInHandler:: Draw ## name ()        \
  {                                               \
    const int count = listDraw ## name.size();    \
    if (count <= 0) {                             \
      return;                                     \
    }                                             \
                                                  \
    LuaOpenGL::EnableDraw ## name ();             \
    listDraw ## name [0]->Draw ## name ();        \
                                                  \
    for (int i = 1; i < count; i++) {             \
      LuaOpenGL::ResetDraw ## name ();            \
      CLuaHandle* lh = listDraw ## name [i];      \
      lh-> Draw ## name ();                       \
    }                                             \
                                                  \
    LuaOpenGL::DisableDraw ## name ();            \
  }

DRAW_CALLIN(World)
DRAW_CALLIN(WorldShadow)
DRAW_CALLIN(WorldReflection)
DRAW_CALLIN(WorldRefraction)
DRAW_CALLIN(Screen)
DRAW_CALLIN(InMiniMap)


/******************************************************************************/
/******************************************************************************/
