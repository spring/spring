#pragma once
#include "InputReceiver.h"

class CResourceBar :
	public CInputReceiver
{
public:
	CResourceBar(void);
	virtual ~CResourceBar(void);
	void Draw(void);

	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);

	bool MousePress(int x, int y, int button);
	void MouseMove(int x, int y, int dx,int dy, int button);

	ContainerBox box;
	ContainerBox metalBox;
	ContainerBox energyBox;

	bool moveBox;
};

extern CResourceBar* resourceBar;
