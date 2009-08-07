#ifndef GLLIST_H
#define GLLIST_H
// glList.h: interface for the CglList class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <boost/function.hpp>

#include "GuiElement.h"

namespace agui
{

class CglList : public GuiElement
{
public:
	CglList(GuiElement* parent = NULL);
	virtual ~CglList();

	// CInputReceiver implementation
	bool KeyPressed(unsigned short k, bool isRepeat);
	bool MousePress(int x, int y, int button);
	void MouseMove(int x, int y, int dx,int dy, int button);
	void MouseRelease(int x, int y, int button);
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);
	std::string GetTooltip(int x,int y) { return tooltip; }

	void AddItem(const std::string& name,const std::string& description);
	int place;
	std::vector<std::string> items;
	std::string name;

	std::string GetCurrentItem() const;
	bool SetCurrentItem(const std::string& item);

	// when attempting to cancel (by pressing escape, clicking outside a button)
	// place is set to cancelPlace (if it's positive) and Select is called.
	int cancelPlace;
	std::string tooltip;

private:
	bool Filter(bool reset);
	void UpOne();
	void DownOne();
	void UpPage();
	void DownPage();
	bool MouseUpdate(int x, int y);

	bool activeMousePress;

	float mx;
	float my;
	float borderSpacing;
	float itemSpacing;
	float itemHeight;

	// for filtering
	std::string query;
	std::vector<std::string>* filteredItems;
	std::vector<std::string> temp1;
	std::vector<std::string> temp2;
};

}

#endif /* GLLIST_H */
