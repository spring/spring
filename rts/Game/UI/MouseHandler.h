/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOUSEHANDLER_H
#define MOUSEHANDLER_H

#include <string>
#include <vector>
#include <map>

#include "System/float3.h"
#include "MouseCursor.h"

static const int NUM_BUTTONS = 10;

class CInputReceiver;
class CCameraController;


class CMouseHandler
{
public:
	CMouseHandler();
	virtual ~CMouseHandler();

	void SetCursor(const std::string& cmdName);

	void UpdateHwCursor(); /// calls SDL_ShowCursor, used for ingame hwcursor enabling

	void EmptyMsgQueUpdate(void);
	void UpdateCursors();
	std::string GetCurrentTooltip(void);

	void HideMouse();
	void ShowMouse();
	void ToggleState(); /// lock+hide (used by fps camera and middle click scrolling)
	void WarpMouse(int x, int y);

	void DrawSelectionBox(); /// draw mousebox (selection box)
	void DrawCursor();

	void MouseRelease(int x, int y, int button);
	void MousePress(int x, int y, int button);
	void MouseMove(int x, int y, int dx, int dy);
	void MouseWheel(float delta);

	bool AssignMouseCursor(const std::string& cmdName,
	                       const std::string& fileName,
	                       CMouseCursor::HotSpot hotSpot,
	                       bool overwrite);
	bool ReplaceMouseCursor(const std::string& oldName,
	                        const std::string& newName,
	                        CMouseCursor::HotSpot hotSpot);

	/// @see ConfigHandler::ConfigNotifyCallback
	void ConfigNotify(const std::string& key, const std::string& value);

	bool hardwareCursor;
	int lastx;
	int lasty;
	bool hide;
	bool hwHide;
	bool locked;
	bool invertMouse;
	float doubleClickTime;
	float scrollWheelSpeed;
	float dragScrollThreshold;

	/// locked mouse indicator size
	float crossSize;
	float crossAlpha;
	float crossMoveScale;

	struct ButtonPressEvt {
		bool pressed;
		bool chorded;
		int x;
		int y;
		float3 camPos;
		float3 dir;
		float time;
		float lastRelease;
		int movement;
	};

	ButtonPressEvt buttons[NUM_BUTTONS + 1]; /// One-bottomed.
	int activeButton;
	float3 dir;

	int soundMultiselID;

	float cursorScale;

	std::string cursorText; /// current cursor name
	CMouseCursor* currentCursor;

	/// Stores if the mouse was locked or not before going into direct control,
	/// so we can restore it when we return to normal.
	bool wasLocked;

	std::map<std::string, CMouseCursor*> cursorFileMap;
	std::map<std::string, CMouseCursor*> cursorCommandMap;

private:
	void SafeDeleteCursor(CMouseCursor* cursor);
	void LoadCursors();

	void DrawScrollCursor();
	void DrawFPSCursor();

	float scrollx;
	float scrolly;
};

extern CMouseHandler* mouse;

#endif /* MOUSEHANDLER_H */
