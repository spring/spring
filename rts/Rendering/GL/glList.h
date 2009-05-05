#ifndef GLLIST_H
#define GLLIST_H
// glList.h: interface for the CglList class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <boost/function.hpp>

#include "Game/UI/InputReceiver.h"

typedef boost::function<void(const std::string&)> ListSelectCallback;

class CglList : public CInputReceiver
{
public:

	// CInputReceiver implementation
	bool KeyPressed(unsigned short k, bool isRepeat);
	bool MousePress(int x, int y, int button);
	void MouseMove(int x, int y, int dx,int dy, int button);
	void MouseRelease(int x, int y, int button);
	bool IsAbove(int x, int y);
	void Draw();
	std::string GetTooltip(int x,int y) { return tooltip; }

	// CglList functions
	void Select();
	void AddItem(const std::string& name,const std::string& description);
	CglList(const std::string& name, ListSelectCallback callback, int id = 0);
	virtual ~CglList();
	int place;
	std::vector<std::string> items;
	std::string name;

	std::string GetCurrentItem() const;

	std::string lastChoosen;
	ListSelectCallback callback;
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

	// GUI
	ContainerBox box;
	bool activeMousePress;

	// used to save default to configHandler
	int id;

	// for filtering
	std::string query;
	std::vector<std::string>* filteredItems;
	std::vector<std::string> temp1;
	std::vector<std::string> temp2;
};

#endif /* GLLIST_H */
