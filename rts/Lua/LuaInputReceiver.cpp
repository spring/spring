#include "StdAfx.h"
// LuaInputReceiver.cpp: implementation of the LuaInputReceiver class.
//
//////////////////////////////////////////////////////////////////////

#include <string>

#include "mmgr.h"

#include "LuaInputReceiver.h"
#include "System/EventHandler.h"


LuaInputReceiver* luaInputReceiver = NULL;


LuaInputReceiver::LuaInputReceiver()
: CInputReceiver(FRONT)
{
}


LuaInputReceiver::~LuaInputReceiver()
{
}


bool LuaInputReceiver::KeyPressed(unsigned short key, bool isRepeat)
{
	return eventHandler.KeyPress(key, isRepeat);
}


bool LuaInputReceiver::KeyReleased(unsigned short key)
{
	return eventHandler.KeyRelease(key);
}


bool LuaInputReceiver::MousePress(int x, int y, int button)
{
	return eventHandler.MousePress(x, y, button);
}


void LuaInputReceiver::MouseMove(int x, int y, int dx, int dy, int button)
{
	eventHandler.MouseMove(x, y, dx, dy, button);
}


void LuaInputReceiver::MouseRelease(int x, int y, int button)
{
	eventHandler.MouseRelease(x, y, button);
}


bool LuaInputReceiver::IsAbove(int x, int y)
{
	return eventHandler.IsAbove(x, y);
}


std::string LuaInputReceiver::GetTooltip(int x, int y)
{
	return eventHandler.GetTooltip(x, y);
}


void LuaInputReceiver::Draw()
{
	return eventHandler.DrawScreen();
}
