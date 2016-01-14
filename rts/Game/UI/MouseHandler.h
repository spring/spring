/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOUSEHANDLER_H
#define MOUSEHANDLER_H

#include <string>
#include <vector>
#include <map>

#include "System/float3.h"
#include "System/type2.h"
#include "MouseCursor.h"

static const int NUM_BUTTONS = 10;

class CInputReceiver;
class CCameraController;
class CUnit;

class CMouseHandler
{
public:
	CMouseHandler();
	virtual ~CMouseHandler();

	void ChangeCursor(const std::string& cmdName, const float& scale = 1.0f);

	void Update();
	void UpdateCursors();
	std::string GetCurrentTooltip();

	void HideMouse();
	void ShowMouse();
	void ToggleMiddleClickScroll(); /// lock+hide
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

	const CMouseCursor* FindCursor(const std::string& cursorName) const {
		std::map<std::string, CMouseCursor*>::const_iterator it = cursorCommandMap.find(cursorName);
		if (it != cursorCommandMap.end()) {
			return it->second;
		}
		return NULL;
	}

	const std::string& GetCurrentCursor() const { return newCursor; }
	const float&  GetCurrentCursorScale() const { return cursorScale; }

	void ToggleHwCursor(const bool& enable);

	/// @see ConfigHandler::ConfigNotifyCallback
	void ConfigNotify(const std::string& key, const std::string& value);

public:
	int lastx;
	int lasty;

	bool locked;

	float doubleClickTime;
	float scrollWheelSpeed;

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

	/// Stores if the mouse was locked or not before going into direct control,
	/// so we can restore it when we return to normal.
	bool wasLocked;

	/// locked mouse indicator size
	float crossSize;
	float crossAlpha;
	float crossMoveScale;

private:
	void SetCursor(const std::string& cmdName, const bool& forceRebind = false);

	void SafeDeleteCursor(CMouseCursor* cursor);
	void LoadCursors();

	static void GetSelectionBoxCoeff(const float3& pos1, const float3& dir1, const float3& pos2, const float3& dir2, float2& topright, float2& btmleft);

	void DrawScrollCursor();
	void DrawFPSCursor();

private:
	std::string newCursor; /// cursor changes are delayed
	std::string cursorText; /// current cursor name
	CMouseCursor* currentCursor;

	float cursorScale;

	bool hide;
	bool hwHide;
	bool hardwareCursor;
	bool invertMouse;

	float dragScrollThreshold;

	float scrollx;
	float scrolly;

	const CUnit* lastClicked;

	std::map<std::string, CMouseCursor*> cursorFileMap;
	std::map<std::string, CMouseCursor*> cursorCommandMap;
};

extern CMouseHandler* mouse;

#endif /* MOUSEHANDLER_H */
