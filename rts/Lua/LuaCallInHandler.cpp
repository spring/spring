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
	mouseOwner = NULL;

	callInMap["GamePreload"]         = &listGamePreload;
	callInMap["GameStart"]           = &listGameStart;
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

	callInMap["UnitEnteredWater"]    = &listUnitEnteredWater;
	callInMap["UnitEnteredAir"]      = &listUnitEnteredAir;
	callInMap["UnitLeftWater"]       = &listUnitLeftWater;
	callInMap["UnitLeftAir"]         = &listUnitLeftAir;

	callInMap["UnitLoaded"]          = &listUnitLoaded;
	callInMap["UnitUnloaded"]        = &listUnitUnloaded;

	callInMap["UnitCloaked"]         = &listUnitCloaked;
	callInMap["UnitDecloaked"]       = &listUnitDecloaked;

	callInMap["FeatureCreated"]      = &listFeatureCreated;
	callInMap["FeatureDestroyed"]    = &listFeatureDestroyed;

	callInMap["ProjectileCreated"]   = &listProjectileCreated;
	callInMap["ProjectileDestroyed"] = &listProjectileDestroyed;

	callInMap["Explosion"]           = &listExplosion;

	callInMap["StockpileChanged"]    = &listStockpileChanged;

	callInMap["Update"]              = &listUpdate;

	callInMap["ViewResize"]          = &listViewResize;

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

  // from LuaUI
  callInMap["KeyPress"]        = &listKeyPress;
  callInMap["KeyRelease"]      = &listKeyRelease;
  callInMap["MouseMove"]       = &listMouseMove;
  callInMap["MousePress"]      = &listMousePress;
  callInMap["MouseRelease"]    = &listMouseRelease;
  callInMap["MouseWheel"]      = &listMouseWheel;
  callInMap["IsAbove"]         = &listIsAbove;
  callInMap["GetTooltip"]      = &listGetTooltip;
  callInMap["CommandNotify"]   = &listCommandNotify;
  callInMap["AddConsoleLine"]  = &listAddConsoleLine;
  callInMap["GroupChanged"]    = &listGroupChanged;
  callInMap["GameSetup"]       = &listGameSetup;
  callInMap["WorldTooltip"]    = &listWorldTooltip;
  callInMap["MapDrawCmd"]      = &listMapDrawCmd;
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
  
	ADDHANDLE(GamePreload);
	ADDHANDLE(GameStart);
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

	ADDHANDLE(UnitEnteredWater);
	ADDHANDLE(UnitEnteredAir);
	ADDHANDLE(UnitLeftWater);
	ADDHANDLE(UnitLeftAir);

	ADDHANDLE(UnitLoaded);
	ADDHANDLE(UnitUnloaded);

	ADDHANDLE(UnitCloaked);
	ADDHANDLE(UnitDecloaked);

	ADDHANDLE(FeatureCreated);
	ADDHANDLE(FeatureDestroyed);

	ADDHANDLE(ProjectileCreated);
	ADDHANDLE(ProjectileDestroyed);

	ADDHANDLE(Explosion);

	ADDHANDLE(StockpileChanged);

	ADDHANDLE(Update);

	ADDHANDLE(ViewResize);

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

  // from LuaUI
  ADDHANDLE(KeyPress);
  ADDHANDLE(KeyRelease);
  ADDHANDLE(MouseMove);
  ADDHANDLE(MousePress);
  ADDHANDLE(MouseRelease);
  ADDHANDLE(MouseWheel);
  ADDHANDLE(IsAbove);
  ADDHANDLE(GetTooltip);
  ADDHANDLE(CommandNotify);
  ADDHANDLE(AddConsoleLine);
  ADDHANDLE(GroupChanged);
  ADDHANDLE(GameSetup);
  ADDHANDLE(WorldTooltip);
  ADDHANDLE(MapDrawCmd);
}


void CLuaCallInHandler::RemoveHandle(CLuaHandle* lh)
{
	if (mouseOwner == lh) {
		// ??
		mouseOwner == NULL;
	}

	ListRemove(handles, lh);

	ListRemove(listGamePreload, lh);
	ListRemove(listGameStart, lh);
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

	ListRemove(listUnitEnteredWater, lh);
	ListRemove(listUnitEnteredAir, lh);
	ListRemove(listUnitLeftWater, lh);
	ListRemove(listUnitLeftAir, lh);

	ListRemove(listUnitLoaded, lh);
	ListRemove(listUnitUnloaded, lh);

	ListRemove(listUnitCloaked, lh);
	ListRemove(listUnitDecloaked, lh);

	ListRemove(listFeatureCreated, lh);
	ListRemove(listFeatureDestroyed, lh);

	ListRemove(listProjectileCreated, lh);
	ListRemove(listProjectileDestroyed, lh);

	ListRemove(listExplosion, lh);

	ListRemove(listStockpileChanged, lh);

	ListRemove(listUpdate, lh);

	ListRemove(listViewResize, lh);

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

  // from LuaUI
  ListRemove(listKeyPress, lh);
  ListRemove(listKeyRelease, lh);
  ListRemove(listMouseMove, lh);
  ListRemove(listMousePress, lh);
  ListRemove(listMouseRelease, lh);
  ListRemove(listMouseWheel, lh);
  ListRemove(listIsAbove, lh);
  ListRemove(listGetTooltip, lh);
  ListRemove(listCommandNotify, lh);
  ListRemove(listAddConsoleLine, lh);
  ListRemove(listGroupChanged, lh);
  ListRemove(listGameSetup, lh);
  ListRemove(listWorldTooltip, lh);
  ListRemove(listMapDrawCmd, lh);
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
	    (ciName == "ViewResize")          ||
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
	    (ciName == "AICallIn")            ||
      // from LuaUI
      (ciName == "KeyPress")            ||
      (ciName == "KeyRelease")          ||
      (ciName == "MouseMove")           ||
      (ciName == "MousePress")          ||
      (ciName == "MouseRelease")        ||
      (ciName == "MouseWheel")          ||
      (ciName == "IsAbove")             ||
      (ciName == "GetTooltip")          ||
      (ciName == "CommandNotify")       ||
      (ciName == "AddConsoleLine")      ||
      (ciName == "GroupChanged")        ||
      (ciName == "GameSetup")           ||
      (ciName == "WorldTooltip")        ||
      (ciName == "MapDrawCmd")) {
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

void CLuaCallInHandler::GamePreload()
{
	const int count = listGamePreload.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listGamePreload[i];
		lh->GamePreload();
	}	
}

void CLuaCallInHandler::GameStart()
{
	const int count = listGameStart.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listGameStart[i];
		lh->GameStart();
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


void CLuaCallInHandler::ViewResize()
{
	const int count = listViewResize.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listViewResize[i];
		lh->ViewResize();
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
//
// from LuaUI
//


static inline bool CheckModUICtrl(const CLuaHandle* lh)
{
	return CLuaHandle::GetModUICtrl() || 
	       CLuaHandle::GetActiveHandle()->GetUserMode();
}




bool CLuaCallInHandler::CommandNotify(const Command& cmd)
{
	// reverse order, user has the override
	const int count = listCommandNotify.size();
	for (int i = (count - 1); i >= 0; i--) {
		CLuaHandle* lh = listCommandNotify[i];
		if (CheckModUICtrl(lh)) {
			if (lh->CommandNotify(cmd)) {
				return true;
			}
		}
	}
	return false;
}


bool CLuaCallInHandler::KeyPress(unsigned short key, bool isRepeat)
{
	// reverse order, user has the override
	const int count = listKeyPress.size();
	for (int i = (count - 1); i >= 0; i--) {
		CLuaHandle* lh = listKeyPress[i];
		if (CheckModUICtrl(lh)) {
			if (lh->KeyPress(key, isRepeat)) {
				return true;
			}
		}
	}
	return false;
}


bool CLuaCallInHandler::KeyRelease(unsigned short key)
{
	// reverse order, user has the override
	const int count = listKeyRelease.size();
	for (int i = (count - 1); i >= 0; i--) {
		CLuaHandle* lh = listKeyRelease[i];
		if (CheckModUICtrl(lh)) {
			if (lh->KeyRelease(key)) {
				return true;
			}
		}
	}
	return false;
}


bool CLuaCallInHandler::MousePress(int x, int y, int button)
{
	// reverse order, user has the override
	const int count = listMousePress.size();
	for (int i = (count - 1); i >= 0; i--) {
		CLuaHandle* lh = listMousePress[i];
		if (CheckModUICtrl(lh)) {
			if (lh->MousePress(x, y, button)) {
				mouseOwner = lh;
				return true;
			}
		}
	}
	return false;
}


// return a cmd index, or -1
int CLuaCallInHandler::MouseRelease(int x, int y, int button)
{
	if (mouseOwner == NULL) {
		return -1;
	}
	const int retval = mouseOwner->MouseRelease(x, y, button);
	mouseOwner = NULL;
	return retval;
}


bool CLuaCallInHandler::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (mouseOwner == NULL) {
		return false;
	}
	return mouseOwner->MouseMove(x, y, dx, dy, button);
}


bool CLuaCallInHandler::MouseWheel(bool up, float value)
{
	// reverse order, user has the override
	const int count = listMouseWheel.size();
	for (int i = (count - 1); i >= 0; i--) {
		CLuaHandle* lh = listMouseWheel[i];
		if (CheckModUICtrl(lh)) {
			if (lh->MouseWheel(up, value)) {
				return true;
			}
		}
	}
	return false;
}


bool CLuaCallInHandler::IsAbove(int x, int y)
{
	// reverse order, user has the override
	const int count = listIsAbove.size();
	for (int i = (count - 1); i >= 0; i--) {
		CLuaHandle* lh = listIsAbove[i];
		if (CheckModUICtrl(lh)) {
			if (lh->IsAbove(x, y)) {
				return true;
			}
		}
	}
	return false;
}


string CLuaCallInHandler::GetTooltip(int x, int y)
{
	// reverse order, user has the override
	const int count = listGetTooltip.size();
	for (int i = (count - 1); i >= 0; i--) {
		CLuaHandle* lh = listGetTooltip[i];
		if (CheckModUICtrl(lh)) {
			const string tt = lh->GetTooltip(x, y);
			if (!tt.empty()) {
				return tt;
			}
		}
	}
	return "";
}


bool CLuaCallInHandler::AddConsoleLine(const string& msg, int zone)
{
	const int count = listAddConsoleLine.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listAddConsoleLine[i];
		if (CheckModUICtrl(lh)) {
			lh->AddConsoleLine(msg, zone);
		}
	}
	return true;
}


bool CLuaCallInHandler::GroupChanged(int groupID)
{
	const int count = listGroupChanged.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listGroupChanged[i];
		if (CheckModUICtrl(lh)) {
			lh->GroupChanged(groupID);
		}
	}
	return false;
}



bool CLuaCallInHandler::GameSetup(const string& state, bool& ready,
                                  const map<int, string>& playerStates)
{
	// reverse order, user has the override
	const int count = listGameSetup.size();
	for (int i = (count - 1); i >= 0; i--) {
		CLuaHandle* lh = listGameSetup[i];
		if (CheckModUICtrl(lh)) {
			if (lh->GameSetup(state, ready, playerStates)) {
				return true;
			}
		}
	}
	return false;
}


string CLuaCallInHandler::WorldTooltip(const CUnit* unit,
                                       const CFeature* feature,
                                       const float3* groundPos)
{
	// reverse order, user has the override
	const int count = listWorldTooltip.size();
	for (int i = (count - 1); i >= 0; i--) {
		CLuaHandle* lh = listWorldTooltip[i];
		if (CheckModUICtrl(lh)) {
			const string tt = lh->WorldTooltip(unit, feature, groundPos);
			if (!tt.empty()) {
				return tt;
			}
		}
	}
	return "";
}


bool CLuaCallInHandler::MapDrawCmd(int playerID, int type,
                                   const float3* pos0, const float3* pos1,
                                   const string* label)
{
	// reverse order, user has the override
	const int count = listMapDrawCmd.size();
	for (int i = (count - 1); i >= 0; i--) {
		CLuaHandle* lh = listMapDrawCmd[i];
		if (CheckModUICtrl(lh)) {
			if (lh->MapDrawCmd(playerID, type, pos0, pos1, label)) {
				return true;
			}
		}
	}
	return false;
}


/******************************************************************************/
/******************************************************************************/
