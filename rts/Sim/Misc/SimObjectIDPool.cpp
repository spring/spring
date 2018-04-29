/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimObjectIDPool.h"
#include "GlobalConstants.h"
#include "GlobalSynced.h"
#include "Sim/Objects/SolidObject.h"
#include "System/creg/STL_Map.h"


CR_BIND(SimObjectIDPool, )
CR_REG_METADATA(SimObjectIDPool, (
	CR_MEMBER(poolIDs),
	CR_MEMBER(freeIDs),
	CR_MEMBER(tempIDs)
))


void SimObjectIDPool::Expand(unsigned int baseID, unsigned int numIDs) {
	std::array<int, (MAX_UNITS > MAX_FEATURES)? MAX_UNITS: MAX_FEATURES> newIDs;

	assert(numIDs <= newIDs.size());

	// allocate new batch of (randomly shuffled) id's
	std::fill(newIDs.begin(), newIDs.end(), 0);
	std::generate(newIDs.begin(), newIDs.begin() + numIDs, [&baseID]() { return (baseID++); });

	// randomize so that Lua widgets can not easily determine counts
	std::random_shuffle(newIDs.begin(), newIDs.begin() + numIDs, gsRNG);
	std::random_shuffle(newIDs.begin(), newIDs.begin() + numIDs, gsRNG);

	// lambda capture ("[n = baseID]() mutable { return (n++); }") requires std=c++14
	baseID -= numIDs;

	// NOTE:
	//   any randomization would be undone by a sorted std::container
	//   instead create a bi-directional mapping from indices to ID's
	//   (where the ID's are a random permutation of the index range)
	//   such that ID's can be assigned and returned to the pool with
	//   their original index, e.g.
	//
	//     freeIDs<idx, uid> = {<0, 13>, < 1, 27>, < 2, 54>, < 3, 1>, ...}
	//     poolIDs<uid, idx> = {<1,  3>, <13,  0>, <27,  1>, <54, 2>, ...}
	//
	//   (the ID --> index map is never changed at runtime!)
	for (unsigned int offsetID = 0; offsetID < numIDs; offsetID++) {
		freeIDs.insert(std::pair<unsigned int, unsigned int>(baseID + offsetID, newIDs[offsetID]));
		poolIDs.insert(std::pair<unsigned int, unsigned int>(newIDs[offsetID], baseID + offsetID));
	}
}



void SimObjectIDPool::AssignID(CSolidObject* object) {
	if (object->id < 0) {
		object->id = ExtractID();
	} else {
		ReserveID(object->id);
	}
}

unsigned int SimObjectIDPool::ExtractID() {
	// extract a random ID from the pool
	//
	// should be unreachable, UnitHandler
	// and FeatureHandler have safeguards
	assert(!IsEmpty());

	const auto it = freeIDs.begin();
	const unsigned int uid = it->second;

	freeIDs.erase(it);

	if (IsEmpty())
		RecycleIDs();

	return uid;
}

void SimObjectIDPool::ReserveID(unsigned int uid) {
	// reserve a chosen ID from the pool
	assert(HasID(uid));
	assert(!IsEmpty());

	const auto it = poolIDs.find(uid);
	const unsigned int idx = it->second;

	freeIDs.erase(idx);

	if (!IsEmpty())
		return;

	RecycleIDs();
}

void SimObjectIDPool::FreeID(unsigned int uid, bool delayed) {
	// put an ID back into the pool either immediately
	// or after all remaining free ID's run out (which
	// is better iff the object count never gets close
	// to the maximum)
	assert(!HasID(uid));

	if (delayed) {
		tempIDs.insert(std::pair<unsigned int, unsigned int>(poolIDs[uid], uid));
	} else {
		freeIDs.insert(std::pair<unsigned int, unsigned int>(poolIDs[uid], uid));
	}
}

bool SimObjectIDPool::RecycleID(unsigned int uid) {
	assert(poolIDs.find(uid) != poolIDs.end());

	const unsigned int idx = poolIDs[uid];
	const auto it = tempIDs.find(idx);

	if (it == tempIDs.end())
		return false;

	tempIDs.erase(idx);
	freeIDs.insert(std::pair<unsigned int, unsigned int>(idx, uid));
	return true;
}

void SimObjectIDPool::RecycleIDs() {
	// throw each ID recycled up until now back into the pool
	freeIDs.insert(tempIDs.begin(), tempIDs.end());
	tempIDs.clear();
}


bool SimObjectIDPool::HasID(unsigned int uid) const {
	assert(poolIDs.find(uid) != poolIDs.end());

	// check if given ID is available (to be assigned) in this pool
	const auto it = poolIDs.find(uid);
	const unsigned int idx = it->second;

	return (freeIDs.find(idx) != freeIDs.end());
}

