#ifndef OBJECT_H
#define OBJECT_H
// Object.h: interface for the CObject class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include "creg/creg.h"
#include "creg/STL_Set.h"

class CObject
{
public:
	CR_DECLARE(CObject);

	void DeleteDeathDependence(CObject* o);
	void AddDeathDependence(CObject* o);
	virtual void DependentDied(CObject* o);
	inline CObject(){};
	virtual ~CObject();
	
	std::multiset<CObject*> listeners,listening;
};

#endif /* OBJECT_H */
