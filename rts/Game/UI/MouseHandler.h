#ifndef MOUSEHANDLER_H
#define MOUSEHANDLER_H
// MouseHandler.h: interface for the CMouseHandler class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <map>

#include "MouseCursor.h"

#define NUM_BUTTONS 5

class CInputReceiver;
class CCameraController;


class CMouseHandler  
{
public:
	void UpdateCursors();
	void HideMouse();
	void ShowMouse();
	void ToggleState();
	void WarpMouse(int x, int y);
	void Draw();
	void MouseRelease(int x,int y,int button);
	void MousePress(int x,int y,int button);
	void MouseMove(int x,int y);
	void MouseWheel(bool up);
	CMouseHandler();
	virtual ~CMouseHandler();

	int lastx;  
	int lasty;  
	bool hide;
	bool locked;
	bool invertMouse;
	float doubleClickTime;
	float scrollWheelSpeed;

	struct ButtonPress {
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

	ButtonPress buttons[NUM_BUTTONS + 1]; /* One-bottomed. */
	float3 dir;

	int activeButton;

	unsigned int cursorTex;
	std::string cursorText;
	std::string cursorTextRight;
	float cursorScale;	
	void DrawCursor(void);
	std::string GetCurrentTooltip(void);

	std::map<std::string, CMouseCursor*> cursorFileMap;
	std::map<std::string, CMouseCursor*> cursorCommandMap;
	//CMouseCursor *mc;

	int soundMultiselID;

	bool AssignMouseCursor(const std::string& cmdName,
	                       const std::string& fileName,
	                       CMouseCursor::HotSpot hotSpot,
	                       bool overwrite);
	bool ReplaceMouseCursor(const std::string& oldName,
	                        const std::string& newName,
	                        CMouseCursor::HotSpot hotSpot);
	void SafeDeleteCursor(CMouseCursor* cursor);

protected:
	void LoadCursors();

public:
	void EmptyMsgQueUpdate(void);

#ifdef DIRECT_CONTROL_ALLOWED
	/* Stores if the mouse was locked or not before going into direct control,
	   so we can restore it when we return to normal. */
	bool wasLocked;
#endif
};

extern CMouseHandler* mouse;

#endif /* MOUSEHANDLER_H */
