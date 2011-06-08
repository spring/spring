/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RESOURCE_BAR_H
#define RESOURCE_BAR_H

#include "InputReceiver.h"

class CResourceBar :
	public CInputReceiver
{
public:
	CResourceBar();
	virtual ~CResourceBar();
	void Draw();

	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);

	bool MousePress(int x, int y, int button);
	void MouseMove(int x, int y, int dx, int dy, int button);

	ContainerBox box;
	ContainerBox metalBox;
	ContainerBox energyBox;

	bool moveBox;
	bool disabled;
};

extern CResourceBar* resourceBar;

#endif /* RESOURCE_BAR_H */
