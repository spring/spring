#ifndef OBJECT_H
#define OBJECT_H
// Object.h: interface for the CObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OBJECT_H__64BC40C1_A468_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_OBJECT_H__64BC40C1_A468_11D4_AD55_0080ADA84DE3__INCLUDED_

#pragma warning(disable:4786)

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#include <set>

class CObject  
{
public:
	void DeleteDeathDependence(CObject* o);
	void AddDeathDependence(CObject* o);
	virtual void DependentDied(CObject* o);
	inline CObject(){};
	virtual ~CObject();
	
	std::set<CObject*> listeners,listening;
};

#endif // !defined(AFX_OBJECT_H__64BC40C1_A468_11D4_AD55_0080ADA84DE3__INCLUDED_)


#endif /* OBJECT_H */
