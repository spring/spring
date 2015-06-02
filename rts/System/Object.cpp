/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <assert.h>
#include "System/Object.h"
#include "System/Log/ILog.h"


CR_BIND(CObject, )

CR_REG_METADATA(CObject, (
	CR_MEMBER(detached),

	CR_IGNORED(listening), //handled in Serialize //FIXME why not use creg serializer?
	CR_IGNORED(listeners), //handled in Serialize

	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
	))


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static void erase_first(CObject::TSyncSafeSet& t, CObject* obj)
{
	auto it = std::find(t.begin(), t.end(), obj);
	//assert(it != t.end());
	if (it != t.end()) t.erase(it);
}

CObject::CObject() : detached(false)
{
}

void CObject::Detach()
{
	// SYNCED
	assert(!detached);
	detached = true;

	for (int depType = 0; depType < listening.size(); ++depType) {
		for (CObject* obj: listeners[depType]) {
			obj->DependentDied(this);
			erase_first(obj->listening[depType], this);
		}

		for (CObject* obj: listening[depType]) {
			erase_first(obj->listeners[depType], this);
		}

		listeners[depType].clear();
		listening[depType].clear();
	}
}


CObject::~CObject()
{
	// UNSYNCED (if detached)
	if (!detached)
		Detach();
}

void CObject::Serialize(creg::ISerializer* ser)
{
	if (ser->IsWriting()) {
		int num = listening.size();
		ser->Serialize(&num, sizeof(int));
		for (int depType = 0; depType < listening.size(); ++depType) {
			int dt = depType;
			ser->Serialize(&dt, sizeof(int));

			TSyncSafeSet& objs = listening[depType];
			int size = 0;
			for (CObject* obj: objs) {
				if (obj->GetClass() != CObject::StaticClass()) {
					size++;
				}
			}
			ser->Serialize(&size, sizeof(int));
			for (CObject* obj: objs) {
				if (obj->GetClass() != CObject::StaticClass()) {
					ser->SerializeObjectPtr((void**)&obj, obj->GetClass());
				} else {
					LOG("Death dependance not serialized in %s", GetClass()->name.c_str());
				}
			}
		}
	} else {
		int num;
		ser->Serialize(&num, sizeof(int));
		for (int i = 0; i < num; i++) {
			int dt;
			ser->Serialize(&dt, sizeof(int));
			int size;
			ser->Serialize(&size, sizeof(int));
			TSyncSafeSet& objs = listening[(DependenceType)dt];
			for (int o = 0; o < size; o++) {
				CObject* obj = NULL;
				ser->SerializeObjectPtr((void**)&*obj, NULL);
				objs.push_back(obj);
			}
		}
	}
}

void CObject::PostLoad()
{
	for (int depType = 0; depType < listening.size(); ++depType) {
		for (CObject* o: listening[depType]) {
			o->listeners[depType].push_back(this);
		}
	}
}

void CObject::DependentDied(CObject* obj)
{
}

// NOTE that we can be listening to a single object from several different places,
// however objects are responsible for not adding the same dependence more than once,
// and preferably try to delete the dependence asap in order not to waste memory
void CObject::AddDeathDependence(CObject* obj, DependenceType dep)
{
	assert(!detached);
	assert(obj != this);

#ifdef DEBUG
	auto it = std::find(listening[dep].begin(), listening[dep].end(), obj);
	assert(it == listening[dep].end());

	auto it2 = std::find(obj->listeners[dep].begin(), obj->listeners[dep].end(), this);
	assert(it2 == obj->listeners[dep].end());
#endif

	listening[dep].push_back(obj);
	obj->listeners[dep].push_back(this);
}


void CObject::DeleteDeathDependence(CObject* obj, DependenceType dep)
{
	assert(!detached);
	assert(obj != this);

	erase_first(listening[dep], obj);
	erase_first(obj->listeners[dep], this);
}
