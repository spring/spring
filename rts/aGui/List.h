/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LIST_H
#define LIST_H

#include <string>
#include <vector>
#include <slimsig/slimsig.h>

#include "GuiElement.h"
#include "System/Misc/SpringTime.h"


namespace agui
{

class List : public GuiElement
{
public:
	List(GuiElement* parent = NULL);
	virtual ~List();

	// CInputReceiver implementation
	bool KeyPressed(int k, bool isRepeat);
	bool MousePress(int x, int y, int button);
	void MouseMove(int x, int y, int dx,int dy, int button);
	void MouseRelease(int x, int y, int button);
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);
	std::string GetTooltip(int x, int y) const { return tooltip; }

	void RemoveAllItems();
	void AddItem(const std::string& name,const std::string& description);
	std::vector<std::string> items;
	std::string name;

	std::string GetCurrentItem() const;
	bool SetCurrentItem(const std::string& item);
	void CenterSelected();

	// when attempting to cancel (by pressing escape, clicking outside a button)
	// place is set to cancelPlace (if it's positive) and Select is called.
	int cancelPlace;
	std::string tooltip;

	slimsig::signal<void (void)> FinishSelection; // Return or Double-Click
	void SetFocus(bool focus);
	void RefreshQuery();

private:
	bool Filter(bool reset);
	void UpOne();
	void DownOne();
	void UpPage();
	void DownPage();
	bool MouseUpdate(int x, int y);
	void UpdateTopIndex();
	void ScrollUpOne();
	void ScrollDownOne();
	int NumDisplay();
	float ScaleFactor();

	spring_time clickedTime;
	int place;

	bool activeMousePress;

	float mx;
	float my;
	float borderSpacing;
	float itemSpacing;
	float itemHeight;
	bool hasFocus;
	int topIndex;

	GuiElement scrollbar;
	bool activeScrollbar;
	float scrollbarGrabPos;

	// for filtering
	std::string query;
	std::vector<std::string>* filteredItems;
	std::vector<std::string> temp1;
	std::vector<std::string> temp2;
};

}

#endif /* GLLIST_H */
