/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaMenuController.h"

#include "Game/UI/MouseHandler.h"
#include "Lua/LuaInputReceiver.h"
#include "Lua/LuaMenu.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Log/ILog.h"

CONFIG(std::string, DefaultMenu).defaultValue("").description("Sets the default menu to be used when spring is started.");

CLuaMenuController* luaMenuController = nullptr;


CLuaMenuController::CLuaMenuController(const std::string& menuName)
{
	SafeDelete(luaMenuController);
	luaMenuController = this;

	// create LuaMenu if necessary
	if (!menuName.empty()) {
		LOG("[%s] using menu: %s", __FUNCTION__, menuName.c_str());
		vfsHandler->AddArchiveWithDeps(menuName, false);
		CLuaMenu::LoadFreeHandler();
	}
	SafeDelete(mouse);
	mouse = new CMouseHandler();
	SafeDelete(luaInputReceiver);
	luaInputReceiver = new LuaInputReceiver();
	mouse->ShowMouse();

}


CLuaMenuController::~CLuaMenuController()
{
	LOG_L(L_WARNING, "deleted blabla");
	SafeDelete(mouse);
	SafeDelete(luaInputReceiver);
}


bool CLuaMenuController::Draw()
{
	spring_msecs(10).sleep();

	eventHandler.Update();
	mouse->Update();
	mouse->UpdateCursors();
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

