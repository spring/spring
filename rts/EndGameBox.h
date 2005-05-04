#pragma once
#include "inputreceiver.h"

class CEndGameBox :
	public CInputReceiver
{
public:
	CEndGameBox(void);
	~CEndGameBox(void);

	virtual bool MousePress(int x, int y, int button);
	virtual void MouseMove(int x, int y, int dx,int dy, int button);
	virtual void MouseRelease(int x, int y, int button);
	virtual void Draw();
	virtual bool IsAbove(int x, int y);
	virtual std::string GetTooltip(int x,int y);

	ContainerBox box;

	ContainerBox exitBox;

	bool moveBox;
};
