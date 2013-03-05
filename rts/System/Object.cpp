/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "System/Object.h"
#include "System/creg/STL_Set.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"


CR_BIND(CObject, )

CR_REG_METADATA(CObject, (
	CR_MEMBER(sync_id),

	CR_MEMBER(detached),

	CR_IGNORED(listening), //handled in Serialize
	CR_IGNORED(listeners), //handled in Serialize

	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
	));

Threading::AtomicCounterInt64 CObject::cur_sync_id(0);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CObject::CObject() : detached(false)
{
	// Note1: this static var is shared between all different types of classes synced & unsynced (CUnit, CFeature, CProjectile, ...)
	//  Still it doesn't break syncness even when synced objects have different sync_ids between clients as long as the sync_id is
	//  creation time dependent and monotonously increasing, so the _order_ remains between clients.

	// Use atomic fetch-and-add, so threads don't read half written data nor write old (= smaller) numbers
	sync_id = ++cur_sync_id;

	assert(sync_id + 1 > sync_id); // check for overflow
}

void CObject::Detach()
{
	// SYNCED
	assert(!detached);
	detached = true;
	for (TDependenceMap::iterator i = listeners.begin(); i != listeners.end(); ++i) {
		const DependenceType& depType = i->first;
		TSyncSafeSet& objs = i->second;

		for (TSyncSafeSet::iterator di = objs.begin(); di != objs.end(); ++di) {
			CObject* const& obj = (*di);

			obj->DependentDied(this);

			assert(obj->listening.find(depType) != obj->listening.end());
			obj->listening[depType].erase(this);
		}
	}
	for (TDependenceMap::iterator i = listening.begin(); i != listening.end(); ++i) {
		const DependenceType& depType = i->first;
		TSyncSafeSet& objs = i->second;

		for (TSyncSafeSet::iterator di = objs.begin(); di != objs.end(); ++di) {
			CObject* const& obj = (*di);

			assert(obj->listeners.find(depType) != obj->listeners.end());
			obj->listeners[depType].erase(this);
		}
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
	//CHECKME this was written when listeners & listening were std::list's, with switching to std::set it would need a rewrite
	if (ser->IsWriting()) {
		int num = listening.size();
		ser->Serialize(&num, sizeof(int));
		for (std::map<DependenceType, TSyncSafeSet >::iterator i = listening.begin(); i != listening.end(); ++i) {
			int dt = i->first;
			ser->Serialize(&dt, sizeof(int));
			TSyncSafeSet& dl = i->second;
			int size = 0;
			TSyncSafeSet::const_iterator oi;
			for (oi = dl.begin(); oi != dl.end(); ++oi) {
				if ((*oi)->GetClass() != CObject::StaticClass()) {
					size++;
				}
			}
			ser->Serialize(&size, sizeof(int));
			for (oi = dl.begin(); oi != dl.end(); ++oi) {
				if ((*oi)->GetClass() != CObject::StaticClass()) {
					ser->SerializeObjectPtr((void**)&*oi, (*oi)->GetClass());
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
			TSyncSafeSet& dl = listening[(DependenceType)dt];
			for (int o = 0; o < size; o++) {
                CObject* obj;
				ser->SerializeObjectPtr((void**)&*obj, NULL);
				dl.insert(obj);
			}
		}
	}
}

void CObject::PostLoad()
{
	//CHECKME this was written when listeners & listening were std::list's, with switching to std::set it would need a rewrite
	for (std::map<DependenceType, TSyncSafeSet >::iterator i = listening.begin(); i != listening.end(); ++i) {
		for (TSyncSafeSet::iterator oi = i->second.begin(); oi != i->second.end(); ++oi) {
			TSyncSafeSet& dl = (*oi)->listeners[i->first];
			dl.insert(this);
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
	listening[dep].insert(obj);

	obj->listeners[dep].insert(this);
}


void CObject::DeleteDeathDependence(CObject* obj, DependenceType dep)
{
	assert(!detached);
	obj->listeners[dep].erase(this);

	listening[dep].erase(obj);
}
