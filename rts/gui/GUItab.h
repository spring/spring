#if !defined(GUITAB_H)
#define GUITAB_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GUIframe.h"
#include <string>
#include <map>
#include <vector>

class GUIbutton;

class GUItab : public GUIframe  
{
public:
	GUItab(int x, int y, int w, int h);
	virtual ~GUItab();

	void AddChild(GUIframe *child);
	void RemoveChild(GUIframe *item);

protected:
	std::map<GUIbutton*, GUIframe*> buttons;
	std::vector<GUIbutton*> order;
	void LayoutButtons();
	void SwitchClicked(GUIbutton* b);
	
	bool EventAction(const std::string& event);
};

#endif // !defined(GUITAB_H)
