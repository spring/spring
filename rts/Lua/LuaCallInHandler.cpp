#include "StdAfx.h"
// LuaCallInHandler.cpp: implementation of the CLuaCallInHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaCallInHandler.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaOpenGL.h"


CLuaCallInHandler luaCallIns;


/******************************************************************************/
/******************************************************************************/

CLuaCallInHandler::CLuaCallInHandler()
{
	callInMap["GameLoadLua"]         = &listGameLoadLua;
	callInMap["GameStartPlaying"]    = &listGameStartPlaying;
	callInMap["GameOver"]            = &listGameOver;
	callInMap["TeamDied"]            = &listTeamDied;

	callInMap["UnitCreated"]         = &listUnitCreated;
	callInMap["UnitFinished"]        = &listUnitFinished;
	callInMap["UnitFromFactory"]     = &listUnitFromFactory;
	callInMap["UnitDestroyed"]       = &listUnitDestroyed;
	callInMap["UnitTaken"]           = &listUnitTaken;
	callInMap["UnitGiven"]           = &listUnitGiven;

	callInMap["UnitIdle"]            = &listUnitIdle;
	callInMap["UnitCmdDone"]         = &listUnitCmdDone;
	callInMap["UnitDamaged"]         = &listUnitDamaged;
	callInMap["UnitExperience"]      = &listUnitExperience;
	callInMap["UnitSeismicPing"]     = &listUnitSeismicPing;
	callInMap["UnitEnteredRadar"]    = &listUnitEnteredRadar;
	callInMap["UnitEnteredLos"]      = &listUnitEnteredLos;
	callInMap["UnitLeftRadar"]       = &listUnitLeftRadar;
	callInMap["UnitLeftLos"]         = &listUnitLeftLos;

	callInMap["UnitLoaded"]          = &listUnitLoaded;
	callInMap["UnitUnloaded"]        = &listUnitUnloaded;

	callInMap["UnitCloaked"]         = &listUnitCloaked;
	callInMap["UnitDecloaked"]       = &listUnitDecloaked;

	callInMap["FeatureCreated"]      = &listFeatureCreated;
	callInMap["FeatureDestroyed"]    = &listFeatureDestroyed;

	callInMap["Explosion"]           = &listExplosion;

	callInMap["StockpileChanged"]    = &listStockpileChanged;

	callInMap["Update"]              = &listUpdate;

	callInMap["DefaultCommand"]      = &listDefaultCommand;

	callInMap["DrawGenesis"]         = &listDrawGenesis;
	callInMap["DrawWorld"]           = &listDrawWorld;
	callInMap["DrawWorldPreUnit"]    = &listDrawWorldPreUnit;
	callInMap["DrawWorldShadow"]     = &listDrawWorldShadow;
	callInMap["DrawWorldReflection"] = &listDrawWorldReflection;
	callInMap["DrawWorldRefraction"] = &listDrawWorldRefraction;
	callInMap["DrawScreenEffects"]   = &listDrawScreenEffects;
	callInMap["DrawScreen"]          = &listDrawScreen;
	callInMap["DrawInMiniMap"]       = &listDrawInMiniMap;
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
  
	ADDHANDLE(GameLoadLua);
	ADDHANDLE(GameStartPlaying);
	ADDHANDLE(GameOver);
	ADDHANDLE(TeamDied);

	ADDHANDLE(UnitCreated);
	ADDHANDLE(UnitFinished);
	ADDHANDLE(UnitFromFactory);
	ADDHANDLE(UnitDestroyed);
	ADDHANDLE(UnitTaken);
	ADDHANDLE(UnitGiven);

	ADDHANDLE(UnitIdle);
	ADDHANDLE(UnitCmdDone);
	ADDHANDLE(UnitDamaged);
	ADDHANDLE(UnitExperience);

	ADDHANDLE(UnitSeismicPing);
	ADDHANDLE(UnitEnteredRadar);
	ADDHANDLE(UnitEnteredLos);
	ADDHANDLE(UnitLeftRadar);
	ADDHANDLE(UnitLeftLos);

	ADDHANDLE(UnitLoaded);
	ADDHANDLE(UnitUnloaded);

	ADDHANDLE(UnitCloaked);
	ADDHANDLE(UnitDecloaked);

	ADDHANDLE(FeatureCreated);
	ADDHANDLE(FeatureDestroyed);

	ADDHANDLE(Explosion);

	ADDHANDLE(StockpileChanged);

	ADDHANDLE(Update);

	ADDHANDLE(DefaultCommand);

	ADDHANDLE(DrawGenesis);
	ADDHANDLE(DrawWorld);
	ADDHANDLE(DrawWorldPreUnit);
	ADDHANDLE(DrawWorldShadow);
	ADDHANDLE(DrawWorldReflection);
	ADDHANDLE(DrawWorldRefraction);
	ADDHANDLE(DrawScreenEffects);
	ADDHANDLE(DrawScreen);
	ADDHANDLE(DrawInMiniMap);
}


void CLuaCallInHandler::RemoveHandle(CLuaHandle* lh)
{
	ListRemove(handles, lh);

	ListRemove(listGameLoadLua, lh);
	ListRemove(listGameStartPlaying, lh);
	ListRemove(listGameOver, lh);
	ListRemove(listTeamDied, lh);

	ListRemove(listUnitCreated, lh);
	ListRemove(listUnitFinished, lh);
	ListRemove(listUnitFromFactory, lh);
	ListRemove(listUnitDestroyed, lh);

	ListRemove(listUnitTaken, lh);
	ListRemove(listUnitGiven, lh);

	ListRemove(listUnitIdle, lh);
	ListRemove(listUnitCmdDone, lh);
	ListRemove(listUnitDamaged, lh);
	ListRemove(listUnitExperience, lh);

	ListRemove(listUnitSeismicPing, lh);
	ListRemove(listUnitEnteredRadar, lh);
	ListRemove(listUnitEnteredLos, lh);
	ListRemove(listUnitLeftRadar, lh);
	ListRemove(listUnitLeftLos, lh);

	ListRemove(listUnitLoaded, lh);
	ListRemove(listUnitUnloaded, lh);

	ListRemove(listUnitCloaked, lh);
	ListRemove(listUnitDecloaked, lh);

	ListRemove(listFeatureCreated, lh);
	ListRemove(listFeatureDestroyed, lh);

	ListRemove(listExplosion, lh);

	ListRemove(listStockpileChanged, lh);

	ListRemove(listUpdate, lh);

	ListRemove(listDefaultCommand, lh);

	ListRemove(listDrawGenesis, lh);
	ListRemove(listDrawWorld, lh);
	ListRemove(listDrawWorldPreUnit, lh);
	ListRemove(listDrawWorldShadow, lh);
	ListRemove(listDrawWorldReflection, lh);
	ListRemove(listDrawWorldRefraction, lh);
	ListRemove(listDrawScreenEffects, lh);
	ListRemove(listDrawScreen, lh);
	ListRemove(listDrawInMiniMap, lh);
}


/******************************************************************************/
/******************************************************************************/

bool CLuaCallInHandler::ManagedCallIn(const string& ciName)
{
	return (callInMap.find(ciName) != callInMap.end());
}


bool CLuaCallInHandler::UnsyncedCallIn(const string& ciName)
{
	if ((ciName == "Update")              ||
	    (ciName == "DefaultCommand")      ||
	    (ciName == "DrawGenesis")         ||
	    (ciName == "DrawWorld")           ||
	    (ciName == "DrawWorldPreUnit")    ||
	    (ciName == "DrawWorldShadow")     ||
	    (ciName == "DrawWorldReflection") ||
	    (ciName == "DrawWorldRefraction") ||
	    (ciName == "DrawScreenEffects")   ||
	    (ciName == "DrawScreen")          ||
	    (ciName == "DrawInMiniMap")       ||
	    (ciName == "DrawUnit")            ||
	    (ciName == "AICallIn")) {
		return true;
	}
	return false;
}


bool CLuaCallInHandler::InsertCallIn(CLuaHandle* lh, const string& ciName)
{
	map<string, CallInList*>::iterator it = callInMap.find(ciName);
	if (it == callInMap.end()) {
		return false;
	}
	ListInsert(*(it->second), lh);
	return true;
}


bool CLuaCallInHandler::RemoveCallIn(CLuaHandle* lh, const string& ciName)
{
	map<string, CallInList*>::iterator it = callInMap.find(ciName);
	if (it == callInMap.end()) {
		return false;
	}
	ListRemove(*(it->second), lh);
	return true;
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

void CLuaCallInHandler::GameLoadLua()
{
	const int count = listGameLoadLua.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listGameLoadLua[i];
		lh->GameLoadLua();
	}	
}

void CLuaCallInHandler::GameStartPlaying()
{
	const int count = listGameStartPlaying.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listGameStartPlaying[i];
		lh->GameStartPlaying();
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
/******************************************************************************/

void CLuaCallInHandler::Update()
{
	const int count = listUpdate.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listUpdate[i];
		lh->Update();
	}
}


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

DRAW_CALLIN(Genesis)
DRAW_CALLIN(World)
DRAW_CALLIN(WorldPreUnit)
DRAW_CALLIN(WorldShadow)
DRAW_CALLIN(WorldReflection)
DRAW_CALLIN(WorldRefraction)
DRAW_CALLIN(ScreenEffects)
DRAW_CALLIN(Screen)
DRAW_CALLIN(InMiniMap)


/******************************************************************************/
/******************************************************************************/
