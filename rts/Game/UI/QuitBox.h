/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _QUIT_BOX_H
#define _QUIT_BOX_H

#include "InputReceiver.h"

class CQuitBox: public CInputReceiver
{
public:
	CQuitBox();

	void Draw() override;

	bool IsAbove(int x, int y) override;
	std::string GetTooltip(int x, int y) override;

	bool MousePress(int x, int y, int button) override;
	void MouseRelease(int x, int y, int button) override;
	void MouseMove(int x, int y, int dx, int dy, int button) override;
	bool KeyPressed(int keyCode, int scanCode, bool isRepeat) override;
private:
	TRectangle<float> box;

	// in order of appearance
	TRectangle<float> resignBox;
	TRectangle<float> saveBox;
	TRectangle<float> giveAwayBox;
	TRectangle<float> teamBox;
	TRectangle<float> menuBox;
	TRectangle<float> quitBox;
	TRectangle<float> cancelBox;
	TRectangle<float> scrollbarBox;
	TRectangle<float> scrollBox;

	int shareTeam;
	bool noAlliesLeft;

	bool moveBox;

	int startTeam;
	int numTeamsDisp;
	bool scrolling;
	float scrollGrab;
	bool hasScroll;
};

#endif // _QUIT_BOX_H

