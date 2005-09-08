#ifndef GLLIST_H
#define GLLIST_H
// glList.h: interface for the CglList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GLLIST_H__87AE2821_660E_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_GLLIST_H__87AE2821_660E_11D4_AD55_0080ADA84DE3__INCLUDED_

#pragma warning(disable:4786)

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

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

#endif // !defined(AFX_GLLIST_H__87AE2821_660E_11D4_AD55_0080ADA84DE3__INCLUDED_)


#endif /* GLLIST_H */
