/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHARE_BOX_H
#define SHARE_BOX_H

#include "InputReceiver.h"

class CShareBox : public CInputReceiver
{
public:
	CShareBox();
	~CShareBox();

	void Draw() override;

	bool IsAbove(int x, int y) override;
	std::string GetTooltip(int x, int y) override;

	bool MousePress(int x, int y, int button) override;
	void MouseRelease(int x, int y, int button) override;
	void MouseMove(int x, int y, int dx, int dy, int button) override;
	bool KeyPressed(int keyCode, int scanCode, bool isRepeat) override;
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
