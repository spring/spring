/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaMenuController.h"

#include "Game/GlobalUnsynced.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/MouseHandler.h"
#include "Lua/LuaInputReceiver.h"
#include "Lua/LuaMenu.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"

CONFIG(std::string, DefaultLuaMenu).defaultValue("").description("Sets the default menu to be used when spring is started.");

CLuaMenuController* luaMenuController = nullptr;


CLuaMenuController::CLuaMenuController(const std::string& menuName)
	: menuArchive(menuName)
	, lastDrawFrameTime(spring_gettime())
{
	if (!Valid())
		menuArchive = configHandler->GetString("DefaultLuaMenu");

	// create LuaMenu if necessary
	if (!Valid())
		return;

	Reset();
	CLuaMenu::LoadFreeHandler();
}

CLuaMenuController::~CLuaMenuController()
{
	CLuaMenu::FreeHandler();
}


bool CLuaMenuController::Reset()
{
	if (!Valid()) {
		// if no LuaMenu, cursor will not be updated (again) until game exists so force a reset
		// calling ReloadCursors here is not possible since no archives are loaded at this point
		mouse->ResetCursor();
		return false;
	}

	LOG("[LuaMenuController::%s] using menu archive \"%s\"", __func__, menuArchive.c_str());

	// lock should not be needed here, but does no harm either
	vfsHandler->GrabLock();
	vfsHandler->SetName("LuaMenuVFS");
	vfsHandler->AddArchiveWithDeps(menuArchive, false);
	vfsHandler->SetName("SpringVFS");
	vfsHandler->FreeLock();

	mouse->ReloadCursors();
	return true;
}

bool CLuaMenuController::Activate(const std::string& msg)
{
	LOG("[LuaMenuController::%s(msg=\"%s\")] luaMenu=%p", __func__, msg.c_str(), luaMenu);

	// LuaMenu might have failed to load, making the controller deadweight
	if (luaMenu == nullptr)
		return false;

	assert(Valid());
	activeController = luaMenuController;

	mouse->ShowMouse();
	luaMenu->ActivateMenu(msg);
	return true;
}

bool CLuaMenuController::ActivateInstance(const std::string& msg)
{
	return (luaMenuController->Valid() && luaMenuController->Activate(msg));
}

void CLuaMenuController::ResizeEvent()
{
	eventHandler.ViewResize();
}




bool CLuaMenuController::Draw()
{
	// we should not become the active controller unless this holds (see ::Activate)
	assert(luaMenu != nullptr);

	eventHandler.CollectGarbage(false);
	infoConsole->PushNewLinesToEventHandler();
	mouse->Update();
	mouse->UpdateCursors();
	eventHandler.Update();
	// calls IsAbove
	mouse->GetCurrentTooltip();

	// render if global rendering active + luamenu allows it, and at least once per 30s
	const bool allowDraw = (globalRendering->active && luaMenu->AllowDraw());
	const bool forceDraw = ((spring_gettime() - lastDrawFrameTime).toSecsi() > 30);

	if (allowDraw || forceDraw) {
		ClearScreen();

		eventHandler.DrawGenesis();
		eventHandler.DrawScreen();
		mouse->DrawCursor();
		eventHandler.DrawScreenPost();

		lastDrawFrameTime = spring_gettime();
		return true;
	}

	spring_msecs(10).sleep(true); // no draw needed, sleep a bit
	return false;
}


int CLuaMenuController::KeyReleased(int k)
{
	luaInputReceiver->KeyReleased(k);
	return 0;
}

int CLuaMenuController::KeyPressed(int k, bool isRepeat)
{
	luaInputReceiver->KeyPressed(k, isRepeat);
	return 0;
}

int CLuaMenuController::TextInput(const std::string& utf8Text)
{
	eventHandler.TextInput(utf8Text);
	return 0;
}

int CLuaMenuController::TextEditing(const std::string& utf8Text, unsigned int start, unsigned int length)
{
	eventHandler.TextEditing(utf8Text, start, length);
	return 0;
}
