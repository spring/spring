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

	enum EObjectType { OT_Unknown, OT_Unsynced, OT_Synced };

	CObject(EObjectType synced = OT_Unknown);
	virtual ~CObject();

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
	std::multiset<CObject*> listeners,listening;
};

#endif /* OBJECT_H */
