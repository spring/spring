#ifndef GLLIST_H
#define GLLIST_H
// glList.h: interface for the CglList class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <vector>

typedef void (* ListSelectCallback) (std::string selected);

class CglList  
{
public:
	void KeyPress(int k);
	void Select();
	void DownOne();
	void UpOne();
	void Draw();
	void AddItem(const char* name,const char* description);
	CglList(const char* name, ListSelectCallback callback, int id = 0);
	virtual ~CglList();
	int place;
	std::vector<std::string> items;
	std::string name;

	std::string lastChoosen;
	ListSelectCallback callback;

private:
	void Filter(bool reset);

	int id;
	std::string query;
	std::vector<std::string>* filteredItems;
	std::vector<std::string> temp1;
	std::vector<std::string> temp2;
};

#endif /* GLLIST_H */
