#ifndef OBJECT_H
#define OBJECT_H
// Object.h: interface for the CObject class.
//
//////////////////////////////////////////////////////////////////////

#include <list>
#include "creg/creg_cond.h"

template<typename T>
void ListErase(std::list<T>& list, const T& what)
{
	typename std::list<T>::iterator it;
	for (it = list.begin(); it != list.end(); ++it) {
		if (*it == what) {
			list.erase(it);
			break;
		}
	}
}

class CObject
{
public:
	CR_DECLARE(CObject);

	CObject();
	virtual ~CObject();
	void Serialize(creg::ISerializer *s);
	void PostLoad();
	
	void DeleteDeathDependence(CObject* o);
	void AddDeathDependence(CObject* o);
	virtual void DependentDied(CObject* o);

private:
	std::list<CObject*> listeners, listening;
};

#endif /* OBJECT_H */
