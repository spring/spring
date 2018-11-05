/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "System/Object.h"
#include "System/ContainerUtil.h"
#include "System/creg/STL_Map.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"

CR_BIND(CObject, )

CR_REG_METADATA(CObject, (
	CR_MEMBER(sync_id),

	CR_MEMBER(detached),

	CR_MEMBER(listening),
	CR_MEMBER(listeners),
	CR_MEMBER(listenersDepTbl),
	CR_MEMBER(listeningDepTbl)
))

std::atomic<std::int64_t> CObject::cur_sync_id(0);



static bool VectorInsertSorted(std::vector<CObject*>& v, CObject* o)
{
	return (spring::VectorInsertUniqueSorted(v, o, [](const CObject* a, const CObject* b) { return (a->GetSyncID() < b->GetSyncID()); }));
}

static bool VectorEraseSorted(std::vector<CObject*>& v, CObject* o)
{
	return (spring::VectorEraseUniqueSorted(v, o, [](const CObject* a, const CObject* b) { return (a->GetSyncID() < b->GetSyncID()); }));
}



CObject::CObject() : detached(false)
{
	// Note1: this static var is shared between all different types of classes synced & unsynced (CUnit, CFeature, CProjectile, ...)
	//  Still it doesn't break syncness even when synced objects have different sync_ids between clients as long as the sync_id is
	//  creation time dependent and monotonously increasing, so the _order_ remains between clients.

	// Use atomic fetch-and-add, so threads don't read half written data nor write old (= smaller) numbers
	sync_id = ++cur_sync_id;

	assert((sync_id + 1) > sync_id); // check for overflow
}


CObject::~CObject()
{
	assert(!detached);
	detached = true;

	for (const auto& p: listenersDepTbl) {
		assert(p.first >= DEPENDENCE_ATTACKER && p.first < DEPENDENCE_COUNT);
		assert(p.second < listeners.size());

		for (CObject* obj: listeners[p.second]) {
			obj->DependentDied(this);

			const auto jt = obj->listeningDepTbl.find(p.first);

			if (jt == obj->listeningDepTbl.end())
				continue;

			VectorEraseSorted(obj->listening[ jt->second ], this);
		}
	}

	for (const auto& p: listeningDepTbl) {
		assert(p.first >= DEPENDENCE_ATTACKER && p.first < DEPENDENCE_COUNT);
		assert(p.second < listening.size());

		for (CObject* obj: listening[p.second]) {
			const auto jt = obj->listenersDepTbl.find(p.first);

			if (jt == obj->listenersDepTbl.end())
				continue;

			VectorEraseSorted(obj->listeners[ jt->second ], this);
		}
	}
}


// NOTE:
//   we can be listening to a single object from several different places
//   objects are responsible for not adding the same dependence more than
//   once, and preferably try to delete the dependence ASAP in order not
//   to waste memory
void CObject::AddDeathDependence(CObject* obj, DependenceType dep)
{
	assert(!detached && !obj->detached);

	// check this explicitly
	if (detached || obj->detached)
		return;

	VectorInsertSorted(const_cast<TSyncSafeSet&>(     GetListening(dep)),  obj);
	VectorInsertSorted(const_cast<TSyncSafeSet&>(obj->GetListeners(dep)), this);
}


void CObject::DeleteDeathDependence(CObject* obj, DependenceType dep)
{
	assert(!detached);

	if (detached || obj->detached)
		return;

	const auto it =      listeningDepTbl.find(dep);
	const auto jt = obj->listenersDepTbl.find(dep);

	if (it !=      listeningDepTbl.end()) VectorEraseSorted(     listening[ it->second ],  obj);
	if (jt != obj->listenersDepTbl.end()) VectorEraseSorted(obj->listeners[ jt->second ], this);
}

