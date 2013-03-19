/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimObjectIDPool.h"
#include "GlobalSynced.h"
#include "Sim/Objects/SolidObject.h"

CR_BIND(SimObjectIDPool, );
CR_REG_METADATA(SimObjectIDPool, (
	CR_MEMBER(liveIdentIndexMap),
	CR_MEMBER(liveIndexIdentMap),
	CR_MEMBER(tempIndexIdentMap)
));

void SimObjectIDPool::Expand(unsigned int baseID, unsigned int numIDs) {
	// allocate new batch of (randomly shuffled) id's
	std::vector<int> newIDs(numIDs);

	for (unsigned int offsetID = 0; offsetID < numIDs; offsetID++) {
		newIDs[offsetID] = baseID + offsetID;
	}

	// randomize so that Lua widgets can not easily determine counts
	SyncedRNG rng;
	std::random_shuffle(newIDs.begin(), newIDs.end(), rng);
	std::random_shuffle(newIDs.begin(), newIDs.end(), rng);

	// NOTE:
	//   any randomization would be undone by using a std::set as-is
	//   instead create a bi-directional mapping from indices to ID's
	//   (where the ID's are a random permutation of the index range)
	//   such that ID's can be assigned and returned to the pool with
	//   their original index
	//
	//     indexIdentMap = {<0, 13>, < 1, 27>, < 2, 54>, < 3, 1>, ...}
	//     identIndexMap = {<1,  3>, <13,  0>, <27,  1>, <54, 2>, ...}
	//
	//   (the ID --> index map is never changed at runtime!)
	for (unsigned int offsetID = 0; offsetID < numIDs; offsetID++) {
		liveIndexIdentMap.insert(IDPair(baseID + offsetID, newIDs[offsetID]));
		liveIdentIndexMap.insert(IDPair(newIDs[offsetID], baseID + offsetID));
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

	const IDMap::iterator it = liveIndexIdentMap.begin();
	const unsigned int id = it->second;

	liveIndexIdentMap.erase(it);

	if (IsEmpty()) {
		RecycleIDs();
	}

	return id;
}

void SimObjectIDPool::ReserveID(unsigned int id) {
	// reserve a chosen ID from the pool
	assert(HasID(id));
	assert(!IsEmpty());

	const IDMap::iterator it = liveIdentIndexMap.find(id);
	const unsigned int idx = it->second;

	liveIndexIdentMap.erase(idx);

	if (IsEmpty()) {
		RecycleIDs();
	}
}

void SimObjectIDPool::FreeID(unsigned int id, bool delayed) {
	// put an ID back into the pool either immediately
	// or after all remaining free ID's run out (which
	// is better iff the object count never gets close
	// to the maximum)
	assert(!HasID(id));

	if (delayed) {
		tempIndexIdentMap.insert(IDPair(liveIdentIndexMap[id], id));
	} else {
		liveIndexIdentMap.insert(IDPair(liveIdentIndexMap[id], id));
	}
}

void SimObjectIDPool::RecycleIDs() {
	// throw each ID recycled up until now back into the pool
	liveIndexIdentMap.insert(tempIndexIdentMap.begin(), tempIndexIdentMap.end());
	tempIndexIdentMap.clear();
}

bool SimObjectIDPool::HasID(unsigned int id) const {
	assert(liveIdentIndexMap.find(id) != liveIdentIndexMap.end());

	// check if given ID is available in this pool
	const IDMap::const_iterator it = liveIdentIndexMap.find(id);
	const unsigned int idx = it->second;

	return (liveIndexIdentMap.find(idx) != liveIndexIdentMap.end());
}

