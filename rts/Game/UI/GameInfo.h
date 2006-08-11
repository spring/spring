#ifndef GAMEINFO_H
#define GAMEINFO_H

#include "InputReceiver.h"

class CGameInfo : public CInputReceiver
{
	public:
		CGameInfo(void);
		~CGameInfo(void);

		bool MousePress(int x, int y, int button);
		bool KeyPressed(unsigned short key);
		void Draw();

	protected:
		ContainerBox box;
};


#endif /* GAMEINFO_H */
