/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_INPUT_RECEIVER
#define LUA_INPUT_RECEIVER

#include <string>

#include "Game/UI/InputReceiver.h"


class LuaInputReceiver : public CInputReceiver
{
	public:
		LuaInputReceiver();
		~LuaInputReceiver();

		bool KeyPressed(unsigned short key, bool isRepeat);
		bool KeyReleased(unsigned short key);

		bool MousePress(int x, int y, int button);
		void MouseMove(int x, int y, int dx, int dy, int button);
		void MouseRelease(int x, int y, int button);
		bool IsAbove(int x, int y);
		std::string GetTooltip(int x,int y);

		void Draw();
};


extern LuaInputReceiver* luaInputReceiver;


#endif /* LUA_INPUT_RECEIVER */

