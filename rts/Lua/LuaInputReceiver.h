/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_INPUT_RECEIVER
#define LUA_INPUT_RECEIVER

#include <string>

#include "Game/UI/InputReceiver.h"


class CLuaInputReceiver : public CInputReceiver
{
	public:
		CLuaInputReceiver();
		~CLuaInputReceiver();

		static CLuaInputReceiver* GetInstace();

		bool KeyPressed(int key, bool isRepeat) override;
		bool KeyReleased(int key) override;

		bool MousePress(int x, int y, int button) override;
		void MouseMove(int x, int y, int dx, int dy, int button) override;
		void MouseRelease(int x, int y, int button) override;
		bool IsAbove(int x, int y) override;
		std::string GetTooltip(int x,int y) override;

		void Draw() override;
};

#define luaInputReceiver CLuaInputReceiver::GetInstace()

#endif /* LUA_INPUT_RECEIVER */

