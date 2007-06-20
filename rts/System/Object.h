#ifndef OBJECT_H
#define OBJECT_H
// Object.h: interface for the CObject class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "creg/creg.h"

#define ListErase(type,_where,what) \
	for (std::list<type>::iterator i=_where.begin();i!=_where.end();i++)\
		if (*i==what) {\
			_where.erase(i);\
			break;\
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
