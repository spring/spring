#include "StdAfx.h"
// LuaInputReceiver.cpp: implementation of the LuaInputReceiver class.
//
//////////////////////////////////////////////////////////////////////

#include <string>

#include "LuaInputReceiver.h"
#include "LuaCallInHandler.h"


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
	return luaCallIns.KeyPress(key, isRepeat);
}


bool LuaInputReceiver::KeyReleased(unsigned short key)
{
	return luaCallIns.KeyRelease(key);
}


bool LuaInputReceiver::MousePress(int x, int y, int button)
{
	return luaCallIns.MousePress(x, y, button);
}


void LuaInputReceiver::MouseMove(int x, int y, int dx, int dy, int button)
{
	luaCallIns.MouseMove(x, y, dx, dy, button);
}


void LuaInputReceiver::MouseRelease(int x, int y, int button)
{
	luaCallIns.MouseRelease(x, y, button);
}


bool LuaInputReceiver::IsAbove(int x, int y)
{
	return luaCallIns.IsAbove(x, y);
}


std::string LuaInputReceiver::GetTooltip(int x, int y)
{
	return luaCallIns.GetTooltip(x, y);
}


void LuaInputReceiver::Draw()
{
	return luaCallIns.DrawScreen();
}
