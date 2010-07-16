/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INPUTRECEIVER_H
#define INPUTRECEIVER_H

#include <deque>
#include <string>

#include "GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"

class CInputReceiver
{
protected:
	enum Where {
		FRONT, BACK
	};

	CInputReceiver(Where w = FRONT);
	virtual ~CInputReceiver();

public:
	virtual bool KeyPressed(unsigned short key, bool isRepeat) { return false; };
	virtual bool KeyReleased(unsigned short key) { return false; };

	virtual bool MousePress(int x, int y, int button) { return false; };
	virtual void MouseMove(int x, int y, int dx, int dy, int button) {};
	virtual void MouseRelease(int x, int y, int button) {};
	virtual bool IsAbove(int x, int y) { return false; };
	virtual void Draw() {};
	virtual std::string GetTooltip(int x,int y){return "No tooltip defined";};

	static void CollectGarbage();
	static CInputReceiver* GetReceiverAt(int x,int y);

	struct ContainerBox {
		ContainerBox();
		ContainerBox operator+(ContainerBox other);
		float x1;
		float y1;
		float x2;
		float y2;
	};
	bool InBox(float x, float y, const ContainerBox& box);
	void DrawBox(const ContainerBox& b, int how = -1);

	// transform from mouse (x,y) to opengl (x,y) (first in screen pixels,
	// second in orthogonal projection 0-1 left-right, bottom-top)
	static float MouseX(int x) { return float(x - globalRendering->viewPosX) / globalRendering->viewSizeX; }
	static float MouseY(int y) { return float(globalRendering->viewSizeY - y) / globalRendering->viewSizeY; }
	static float MouseMoveX(int x) { return float(x) / globalRendering->viewSizeX; }
	static float MouseMoveY(int y) { return -float(y) / globalRendering->viewSizeY; }

	static float guiAlpha;

	static CInputReceiver*& GetActiveReceiverRef() { return activeReceiver; }

protected:
	static CInputReceiver* activeReceiver;
};

std::deque<CInputReceiver*>& GetInputReceivers();

#endif /* INPUTRECEIVER_H */

