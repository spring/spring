/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaMenuController.h"

#include "Game/UI/MouseHandler.h"
#include "Lua/LuaInputReceiver.h"
#include "Lua/LuaMenu.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Log/ILog.h"

CONFIG(std::string, DefaultLuaMenu).defaultValue("").description("Sets the default menu to be used when spring is started.");

CLuaMenuController* luaMenuController = nullptr;


CLuaMenuController::CLuaMenuController(const std::string& menuName)
 : menuArchive(menuName)
{
	if (menuArchive.empty())
		menuArchive = configHandler->GetString("DefaultLuaMenu");

	// create LuaMenu if necessary
	if (!menuArchive.empty()) {
		Reset();
		CLuaMenu::LoadFreeHandler();
	}
}


void CLuaMenuController::Reset()
{
	if (!Valid())
		return;

	LOG("[%s] using menu: %s", __FUNCTION__, menuArchive.c_str());
	vfsHandler->AddArchiveWithDeps(menuArchive, false);

	if (mouse == nullptr)
		mouse = new CMouseHandler();

	if (luaInputReceiver == nullptr)
		luaInputReceiver = new LuaInputReceiver();
}

void CLuaMenuController::Activate()
{
	assert(luaMenuController != nullptr && Valid());
	activeController = luaMenuController;
	mouse->ShowMouse();
}


CLuaMenuController::~CLuaMenuController()
{
	SafeDelete(mouse);
	SafeDelete(luaInputReceiver);
}


bool CLuaMenuController::Draw()
{
	spring_msecs(10).sleep();

	configHandler->Update();
	mouse->Update();
	mouse->UpdateCursors();
	eventHandler.Update();

	// calls IsAbove
	mouse->GetCurrentTooltip();

	ClearScreen();
	eventHandler.DrawGenesis();
	eventHandler.DrawScreen();
	mouse->DrawCursor();
	return true;
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

