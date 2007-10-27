#ifndef OBJECT_H
#define OBJECT_H
// Object.h: interface for the CObject class.
//
//////////////////////////////////////////////////////////////////////

#include <list>
#include "creg/creg.h"

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

	enum EObjectType { OT_Unknown, OT_Unsynced, OT_Synced };

	CObject(EObjectType synced = OT_Unknown);
	virtual ~CObject();
	void Serialize(creg::ISerializer *s);
	void PostLoad();

	static CObject* GetSyncedObjects() { return syncedObjects; }
	CObject* GetNext()     const { return next; }
	CObject* GetPrevious() const { return prev; }
	bool IsSynchronized()  const { return prev || next; }

	void DeleteDeathDependence(CObject* o);
	void AddDeathDependence(CObject* o);
	virtual void DependentDied(CObject* o);

private:
	static CObject* syncedObjects;
	CObject *prev, *next;
	std::list<CObject*> listeners,listening;
};

#endif /* OBJECT_H */
