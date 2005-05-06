#pragma once
#include "InputReceiver.h"

class CStartPosSelecter :
	public CInputReceiver
{
public:
	CStartPosSelecter(void);
	~CStartPosSelecter(void);

	virtual bool MousePress(int x, int y, int button);
	virtual void Draw();

	ContainerBox readyBox;
};

