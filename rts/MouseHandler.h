// MouseHandler.h: interface for the CMouseHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MOUSEHANDLER_H__E6FA621D_050F_430D_B49B_A1E7FA1C1BDF__INCLUDED_)
#define AFX_MOUSEHANDLER_H__E6FA621D_050F_430D_B49B_A1E7FA1C1BDF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <string>
#include <map>

#include "MouseCursor.h"

class CInputReceiver;
class CCameraController;

class CMouseHandler  
{
public:
	void UpdateCam();
	void ToggleState(bool shift);
	void HideMouse();
	void ShowMouse();
	void Draw();
	void MouseRelease(int x,int y,int button);
	void MousePress(int x,int y,int button);
	void MouseMove(int x,int y);
	CMouseHandler();
	virtual ~CMouseHandler();

	int lastx;  
	int lasty;  
	bool hide;
	bool locked;
	bool inStateTransit;
	bool invertMouse;
	double transitSpeed;

	struct ButtonPress{
		bool pressed;
		int x;
		int y;
		float3 camPos;
		float3 dir;
		double time;
		double lastRelease;
		int movement;
	};

	ButtonPress buttons[5];
	float3 dir;

	CInputReceiver* activeReceiver;
	int activeButton;

	unsigned int cursorTex;
	std::string cursorText;
	std::string cursorTextRight;
	void DrawCursor(void);
	std::string GetCurrentTooltip(void);

	map<std::string, CMouseCursor *> cursors;
	//CMouseCursor *mc;

	CCameraController* currentCamController;
	std::vector<CCameraController*> camControllers;
	int currentCamControllerNum;

protected:
	int soundMultiselID;
public:
	void EmptyMsgQueUpdate(void);
};

extern CMouseHandler* mouse;

#endif // !defined(AFX_MOUSEHANDLER_H__E6FA621D_050F_430D_B49B_A1E7FA1C1BDF__INCLUDED_)

