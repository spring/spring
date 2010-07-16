/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHAREBOX_H
#define SHAREBOX_H

#include "InputReceiver.h"

class CShareBox :
	public CInputReceiver
{
public:
	CShareBox(void);
	~CShareBox(void);

	void Draw(void);

	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);

	bool MousePress(int x, int y, int button);
	void MouseRelease(int x,int y,int button);
	void MouseMove(int x, int y, int dx,int dy, int button);
	bool KeyPressed(unsigned short key, bool isRepeat);

	ContainerBox box;

	ContainerBox okBox;
	ContainerBox cancelBox;
	ContainerBox applyBox;
	ContainerBox teamBox;
	ContainerBox unitBox;
	ContainerBox metalBox;
	ContainerBox energyBox;
	
	int shareTeam;
	static int lastShareTeam;

	float metalShare;
	float energyShare;
	bool shareUnits;

	bool energyMove;
	bool metalMove;
	bool moveBox;
};


#endif /* SHAREBOX_H */
