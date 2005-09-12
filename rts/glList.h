#ifndef GLLIST_H
#define GLLIST_H
// glList.h: interface for the CglList class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <vector>
#ifdef _WIN32
#ifndef _WINSOCKAPI_
	#define _WINSOCKAPI_
	#include <windows.h>
	#undef _WINSOCKAPI_
#else
	#include <windows.h>
#endif
#endif

typedef void (* ListSelectCallback) (std::string selected);

class CglList  
{
public:
	void Select();
	void DownOne();
	void UpOne();
	void Draw();
	void AddItem(const char* name,const char* description);
	CglList(const char* name,ListSelectCallback callback);
	virtual ~CglList();
	int place;
	std::vector<std::string> items;
	std::string name;

	std::string lastChoosen;
	ListSelectCallback callback;
};

#endif /* GLLIST_H */
