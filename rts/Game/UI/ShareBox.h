/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHARE_BOX_H
#define SHARE_BOX_H

#include "InputReceiver.h"

class CShareBox : public CInputReceiver
{
public:
	CShareBox();
	~CShareBox();

	void Draw();

	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);

	bool MousePress(int x, int y, int button);
	void MouseRelease(int x, int y, int button);
	void MouseMove(int x, int y, int dx, int dy, int button);
	bool KeyPressed(int keyCode, int scanCode, bool isRepeat);

private:
	ContainerBox box;

	ContainerBox okBox;
	ContainerBox cancelBox;
	ContainerBox applyBox;
	ContainerBox teamBox;
	ContainerBox unitBox;
	ContainerBox metalBox;
	ContainerBox energyBox;
	ContainerBox scrollbarBox;
	ContainerBox scrollBox;

	int shareTeam;
	static int lastShareTeam;

	float metalShare;
	float energyShare;
	bool shareUnits;

	bool energyMove;
	bool metalMove;
	bool moveBox;

	int startTeam;
	int numTeamsDisp;
	bool scrolling;
	float scrollGrab;
	bool hasScroll;
};

#endif /* SHARE_BOX_H */
