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
	bool KeyPressed(int key, bool isRepeat);

private:
	TRectangle<float> box;

	TRectangle<float> okBox;
	TRectangle<float> cancelBox;
	TRectangle<float> applyBox;
	TRectangle<float> teamBox;
	TRectangle<float> unitBox;
	TRectangle<float> metalBox;
	TRectangle<float> energyBox;
	TRectangle<float> scrollbarBox;
	TRectangle<float> scrollBox;

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
