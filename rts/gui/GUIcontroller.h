#ifndef GUICONTROLLER_H
#define GUICONTROLLER_H

#include <string>
#include <map>
#include "command.h"
#include "float3.h"
#include "SelectionKeyHandler.h"

class CSunParser;
class GUIframe;
class GUIbutton;
class GUIstateButton;
class CFeature;
class CUnit;
class float3;
struct UnitDef;
class GUIgame;
class GUIconsole;
class GUItable;

class GUIdialogController
{
public:
	GUIdialogController() {}
	virtual ~GUIdialogController() {}
	
	virtual void DialogEvent(const std::string& event) {}
protected:
	virtual	void ButtonPressed(GUIbutton* b) {}
	virtual void StateChanged(GUIstateButton*, int) {}
	virtual void BuildMenuClicked(UnitDef* ud) {}
	virtual void ConsoleInput(const std::string& text) {}
public:	//generate error in guisharingdialog otherwise
	virtual void TableSelection(GUItable* table, int i,int button) {}
protected:
	
	virtual GUIframe* CreateControl(const std::string& type, int x, int y, int w, int h, CSunParser& parser)
	{
		return NULL;
	}
	
	void CreateUIElement(CSunParser& parser, GUIframe* parent, const std::string& path);
	
	std::map<std::string, GUIframe*> controls;
};

class GUIcontroller : public GUIdialogController
{
public:
	GUIcontroller();
	virtual ~GUIcontroller();

	static bool MouseDown(int x1, int y1, int button);
	static bool MouseUp(int x1, int y1, int button);
	static bool MouseMove(int x1, int y1, int xrel, int yrel, int buttons);

	static void KeyUp(int key);
	static void KeyDown(int key);
	static void Character(char c);

	
	static bool Event(const std::string& event);
	
	static void Draw();
	
	void UpdateCommands();
	
	static const string Modifiers();
	static const string KeyName(unsigned char k);
	
	void LoadInterface(const std::string& name);
	
	Command GetOrderPreview();

	void AddText(const char* fmt, ...);
	void AddText(const std::string& text);
	void SetLastMsgPos(float3 pos);
	float3 lastMsgPos;
	GUIcontroller& GUIcontroller::operator<< (const char* c);
	GUIcontroller& GUIcontroller::operator<< (int i);
	GUIcontroller& GUIcontroller::operator<< (float f);


protected:
	void ButtonPressed(GUIbutton* b);
	void StateChanged(GUIstateButton*, int);
	void BuildMenuClicked(UnitDef* ud);
	void ConsoleInput(const std::string& text);
	void TableSelection(GUItable* table, int i,int button);
	
	GUIframe* CreateControl(const std::string& type, int x, int y, int w, int h, CSunParser& parser);
	
	GUIgame* gameControl;
	
	virtual bool EventAction(const std::string& event);
	
	GUIconsole *console;
	CSelectionKeyHandler		m_SelectionKeyHandler;
	std::string tempstring;
};

extern GUIcontroller* guicontroller;

#endif	// GUICONTROLLER_H
