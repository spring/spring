#ifndef LIST_H
#define LIST_H

#include <string>
#include <vector>
#include <boost/signal.hpp>

#include "GuiElement.h"

namespace agui
{

class List : public GuiElement
{
public:
	List(GuiElement* parent = NULL);
	virtual ~List();

	// CInputReceiver implementation
	bool KeyPressed(unsigned short k, bool isRepeat);
	bool MousePress(int x, int y, int button);
	void MouseMove(int x, int y, int dx,int dy, int button);
	void MouseRelease(int x, int y, int button);
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);
	std::string GetTooltip(int x,int y) { return tooltip; }

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

	boost::signal<void (void)> FinishSelection; // Return or Double-Click
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

	unsigned clickedTime;
	int place;

	bool activeMousePress;

	float mx;
	float my;
	float borderSpacing;
	float itemSpacing;
	float itemHeight;
	bool hasFocus;
	int topIndex;

	// for filtering
	std::string query;
	std::vector<std::string>* filteredItems;
	std::vector<std::string> temp1;
	std::vector<std::string> temp2;
};

}

#endif /* GLLIST_H */
