#include "stdafx.h"
// Object.cpp: implementation of the CObject class.
//
//////////////////////////////////////////////////////////////////////

#include "Object.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CObject::~CObject()
{
	std::set<CObject*>::iterator di;
	for(di=listeners.begin();di!=listeners.end();++di){
		(*di)->DependentDied(this);
		(*di)->listening.erase(this);
	}
	for(di=listening.begin();di!=listening.end();++di){
		(*di)->listeners.erase(this);
	}
}

void CObject::DependentDied(CObject* o)
{
//	MessageBox(0,"wrong","ag",0);
}

void CObject::AddDeathDependence(CObject *o)
{
	o->listeners.insert(this);
	listening.insert(o);
}

void CObject::DeleteDeathDependence(CObject *o)
{
	listening.erase(o);
	o->listeners.erase(this);
}
