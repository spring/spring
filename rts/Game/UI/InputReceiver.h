/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INPUT_RECEIVER_H
#define INPUT_RECEIVER_H

#include <list>
#include <string>

#include "Rendering/GlobalRendering.h"

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
	static CInputReceiver* GetReceiverAt(int x, int y);

	struct ContainerBox {
		ContainerBox();
		ContainerBox operator+(ContainerBox other) const;
		float x1;
		float y1;
		float x2;
		float y2;
	};
	bool InBox(float x, float y, const ContainerBox& box) const;
	void DrawBox(const ContainerBox& b, int how = -1);

	/// Transform from mouse X to OpenGL X value in screen pixels.
	static float MouseX(int x) {
		return float(x - globalRendering->viewPosX) / globalRendering->viewSizeX;
	}
	/// Transform from mouse Y to OpenGL Y value in screen pixels.
	static float MouseY(int y) {
		return float(globalRendering->viewSizeY - y) / globalRendering->viewSizeY;
	}
	/**
	 * Transform from mouse X to OpenGL X value
	 * in orthogonal projection 0-1 left-right.
	 */
	static float MouseMoveX(int x) {
		return float(x) / globalRendering->viewSizeX;
	}
	/**
	 * Transform from mouse Y to OpenGL Y value
	 * in orthogonal projection 0-1 bottom-top.
	 */
	static float MouseMoveY(int y) {
		return -float(y) / globalRendering->viewSizeY;
	}

	static float guiAlpha;

	static CInputReceiver*& GetActiveReceiverRef() { return activeReceiver; }

protected:
	static CInputReceiver* activeReceiver;
};

std::list<CInputReceiver*>& GetInputReceivers();

#endif /* INPUT_RECEIVER_H */

