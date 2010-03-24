/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAMEINFO_H
#define GAMEINFO_H

#include "InputReceiver.h"

class CGameInfo : public CInputReceiver
{
	public:
		static void Enable();
		static void Disable();
		static bool IsActive();

	protected:
		CGameInfo(void);
		~CGameInfo(void);

		bool MousePress(int x, int y, int button);
		void MouseRelease(int x, int y, int button);
		bool KeyPressed(unsigned short key, bool isRepeat);
		bool IsAbove(int x, int y);
		std::string GetTooltip(int x,int y);
		void Draw();

	protected:
		ContainerBox box;

	protected:
		static CGameInfo* instance;
};

#endif /* GAMEINFO_H */
