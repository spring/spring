/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INPUT_RECEIVER_H
#define INPUT_RECEIVER_H

#include <deque>
#include <string>

#include "Rendering/GlobalRendering.h"
#include "System/Rectangle.h"

class CInputReceiver
{
protected:
	enum Where {
		FRONT,
		BACK,
		MANUAL
	};

	CInputReceiver(Where w = FRONT);
	virtual ~CInputReceiver();

public:
	virtual bool KeyPressed(int key, bool isRepeat) { return false; }
	virtual bool KeyReleased(int key) { return false; }

	virtual bool MousePress(int x, int y, int button) { return false; }
	virtual void MouseMove(int x, int y, int dx, int dy, int button) {}
	virtual void MouseRelease(int x, int y, int button) {}
	virtual bool IsAbove(int x, int y) { return false; }
	virtual void Draw() {}
	virtual std::string GetTooltip(int x, int y) { return "No tooltip defined"; }

	static void CollectGarbage();
	static void DrawReceivers();

	bool InBox(float x, float y, const TRectangle<float>& box) const;

	// transform from mouse X/Y to OpenGL X/Y value in screen pixels
	static float MouseX(int x) { return (float(x - globalRendering->viewPosX     ) * globalRendering->pixelX); }
	static float MouseY(int y) { return (float(    globalRendering->viewSizeY - y) * globalRendering->pixelY); }

	// transform from mouse X/Y to OpenGL X/Y value in
	// orthogonal projection 0-1 left-right/bottom-top
	static float MouseMoveX(int x) { return ( float(x) * globalRendering->pixelX); }
	static float MouseMoveY(int y) { return (-float(y) * globalRendering->pixelY); }

	static float guiAlpha;

	static CInputReceiver* GetReceiverAt(int x, int y);
	static CInputReceiver*& GetActiveReceiverRef() { return activeReceiver; }

	static std::deque<CInputReceiver*>& GetReceivers() {
		// This construct fixes order of initialization between different
		// compilation units using inputReceivers. (mantis #34)
		static std::deque<CInputReceiver*> sInputReceivers;
		return sInputReceivers;
	}

protected:
	static CInputReceiver* activeReceiver;
};

#endif /* INPUT_RECEIVER_H */

