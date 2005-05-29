#if !defined(GUISWITCHBAR_H)
#define GUISWITCHBAR_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GUIframe.h"
#include <string>
#include <map>

class GUIswitchableCaption
{
public:
	virtual const string GetCaption()=0;
};

class GUIbutton;

class GUIswitchBar : public GUIframe  
{
public:
	GUIswitchBar();
	virtual ~GUIswitchBar();

	void AddSwitchableChild(GUIframe *child, const string& caption="");
	void RemoveChild(GUIframe *item);
	
	void SwitchClicked(GUIbutton* button);
protected:
	std::map<GUIbutton*, GUIframe*> buttons;
	void LayoutButtons();
	GUIframe *embedded;
};

#endif // !defined(GUISWITCHBAR_H)
