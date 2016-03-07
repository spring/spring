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
	))

Threading::AtomicCounterInt64 CObject::cur_sync_id(0);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CObject::CObject() : detached(false), listeners(), listening()
{
	// Note1: this static var is shared between all different types of classes synced & unsynced (CUnit, CFeature, CProjectile, ...)
	//  Still it doesn't break syncness even when synced objects have different sync_ids between clients as long as the sync_id is
	//  creation time dependent and monotonously increasing, so the _order_ remains between clients.

	// Use atomic fetch-and-add, so threads don't read half written data nor write old (= smaller) numbers
	sync_id = ++cur_sync_id;

	assert(sync_id + 1 > sync_id); // check for overflow
}


CObject::~CObject()
{
	assert(!detached);
	detached = true;
	for (int depType = 0; depType < DEPENDENCE_COUNT; ++depType) {
		if (!listeners[depType])
			continue;

		for (CObject* obj: *listeners[depType]) {
			obj->DependentDied(this);

			assert(obj->listening[depType]);
			obj->listening[depType]->erase(this);
		}
		delete listeners[depType];
	}
	for (int depType = 0; depType < DEPENDENCE_COUNT; ++depType) {
		if (!listening[depType])
			continue;

		for (CObject* obj: *listening[depType]) {
			assert(obj->listeners[depType]);
			obj->listeners[depType]->erase(this);
		}
		delete listening[depType];
	}
}

#ifdef USING_CREG
void CObject::Serialize(creg::ISerializer* ser)
{
	if (ser->IsWriting()) {
		int num = 0;
		for (int dt = 0; dt < DEPENDENCE_COUNT; ++dt) {
			if (listening[dt]) {
				++num;
			}
		}
		ser->Serialize(&num, sizeof(int));
		for (int dt = 0; dt < DEPENDENCE_COUNT; ++dt) {
			if (listening[dt]) {
				ser->Serialize(&dt, sizeof(int));
				TSyncSafeSet& dl = *listening[dt];
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
		}
	} else {
		int num;
		ser->Serialize(&num, sizeof(int));
		for (int i = 0; i < num; i++) {
			int dt;
			ser->Serialize(&dt, sizeof(int));
			int size;
			ser->Serialize(&size, sizeof(int));
			if (!listening[dt])
				listening[dt] = new TSyncSafeSet();
			TSyncSafeSet& dl = *listening[dt];
			for (int o = 0; o < size; o++) {
				CObject* obj = NULL;
				ser->SerializeObjectPtr((void**)&obj, NULL);
				dl.insert(obj);
			}
		}
		// cur_sync_id needs fixing since it's not serialized
		// it might not be exactly where we left it, but that's not important
		// since only order matters
		cur_sync_id = Threading::AtomicCounterInt64(std::max(sync_id, (boost::int64_t) cur_sync_id));
	}
}

void CObject::PostLoad()
{
	for (int depType = 0; depType < DEPENDENCE_COUNT; ++depType) {
		if (!listening[depType])
			continue;

		for (CObject* obj: *listening[depType]) {
			if (!obj->listeners[depType])
				obj->listeners[depType] = new TSyncSafeSet();

			obj->listeners[depType]->insert(this);
		}
	}
}

#endif //USING_CREG

void CObject::DependentDied(CObject* obj)
{
}

// NOTE that we can be listening to a single object from several different places,
// however objects are responsible for not adding the same dependence more than once,
// and preferably try to delete the dependence asap in order not to waste memory
void CObject::AddDeathDependence(CObject* obj, DependenceType dep)
{
	assert(!detached && !obj->detached);
	if (!listening[dep])
		listening[dep] = new TSyncSafeSet();

	listening[dep]->insert(obj);

	if (!obj->listeners[dep])
		obj->listeners[dep] = new TSyncSafeSet();

	obj->listeners[dep]->insert(this);
}


void CObject::DeleteDeathDependence(CObject* obj, DependenceType dep)
{
	assert(!detached);
	if (obj->detached)
		return;

	obj->listeners[dep]->erase(this);

	listening[dep]->erase(obj);
}
