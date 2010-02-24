/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STARTPOSSELECTER_H
#define STARTPOSSELECTER_H

#include "InputReceiver.h"
#include "float3.h"

class CStartPosSelecter :
	public CInputReceiver
{
public:
	CStartPosSelecter(void);
	~CStartPosSelecter(void);

	virtual bool MousePress(int x, int y, int button);
	virtual void Draw();

	bool Ready();
	void ShowReady(bool value) { showReady = value; }

	static CStartPosSelecter* selector;

private:
	bool showReady;
	ContainerBox readyBox;

	bool startPosSet;
	float3 startPos;
};


#endif /* STARTPOSSELECTER_H */
