#if !defined(GUICENTERBUILDMENU_H)
#define GUICENTERBUILDMENU_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GUIframe.h"
#include "Functor.h"

#include <string>
#include <vector>
#include "guibuildmenu.h"
class GUIlabel;
class GUIbutton;
struct UnitDef;
struct CommandDescription;

class GUIcenterBuildMenu : public GUIbuildMenu
{
public:
	GUIcenterBuildMenu(int x, int y, int w, int h, Functor1<UnitDef*> select);
	virtual ~GUIcenterBuildMenu();
	
	void Move(GUIbutton* button);
	
	void SetBuildPicSize(int size) { buildPicSize=size; }
protected:
	void PrivateDraw();
	bool MouseUpAction(int x, int y, int button);
	bool MouseDownAction(int x, int y, int button);
	bool MouseMoveAction(int x, int y, int xrel, int yrel, int button);
	bool EventAction(const std::string& event);

	int PicForPos(int x, int y);
	
	string Tooltip();

	bool rehideMouse;
};

#endif // !defined(GUIBUILDMENU_H)
