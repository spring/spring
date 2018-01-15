/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STARTPOSSELECTER_H
#define STARTPOSSELECTER_H

#include "InputReceiver.h"
#include "System/float3.h"

class CStartPosSelecter: public CInputReceiver
{
public:
	CStartPosSelecter();
	~CStartPosSelecter();

	virtual bool MousePress(int x, int y, int button);
	virtual void Draw();

	bool Ready(bool luaForcedReady);
	void ShowReadyBox(bool b) { showReadyBox = b; }

	static CStartPosSelecter* GetSelector() { return selector; }

private:
	void DrawStartBox() const;

private:
	TRectangle<float> readyBox;
	float3 setStartPos;

	bool showReadyBox;
	bool startPosSet;

	static CStartPosSelecter* selector;
};


#endif /* STARTPOSSELECTER_H */
