#ifndef OBJECT_H
#define OBJECT_H
// Object.h: interface for the CObject class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

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

#endif /* OBJECT_H */
