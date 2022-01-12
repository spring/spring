/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RESOURCE_BAR_H
#define RESOURCE_BAR_H

#include "InputReceiver.h"

class CResourceBar :
	public CInputReceiver
{
public:
	CResourceBar();
	void Draw() override;

	bool IsAbove(int x, int y) override;
	std::string GetTooltip(int x, int y) override;

	bool MousePress(int x, int y, int button) override;
	void MouseMove(int x, int y, int dx, int dy, int button) override;

	TRectangle<float> box;
	TRectangle<float> metalBox;
	TRectangle<float> energyBox;

	bool moveBox;
	bool enabled;
};

extern CResourceBar* resourceBar;

#endif /* RESOURCE_BAR_H */
